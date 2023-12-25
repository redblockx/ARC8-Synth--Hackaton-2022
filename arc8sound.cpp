#include "daisy_seed.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace daisysp;

// Number of octaves
#define NO_OCTAVES     4
// Number of oscillators, one for each tone
#define NO_OSCILLATORS 12
// Fundamental frequency
float freq_0 = 440;
// Global variable to retain the current octave
int octave = 0; 
DaisySeed hw;
Mpr121I2C mpr;
Oscillator osc[NO_OSCILLATORS];
static AdEnv env[NO_OSCILLATORS];
static Autowah    autowah;
volatile uint16_t g_MPR121_state = 0;
NeoTrellisI2C neo;
//                 		 R    G    B
uint32_t colors[][3] = {{255, 0, 0},
					    {0,  255, 0},
						{0, 0, 255},
						{255, 255, 0}}; 



void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
	float osc_out = 0;
	float env_out = 0;
	float frequency = 0;

	for(int i = 0; i < NO_OSCILLATORS; i++)
	{
		if (((g_MPR121_state >> i) & 0x01) && (!env[i].GetCurrentSegment()))
		{
			env[i].Trigger();
			frequency = float(freq_0 * float(powf(2, float(i)/12) * powf(2, octave)));
			osc[i].SetFreq(frequency);
		}
	}

	for (size_t i = 0; i < size; i+=2)
	{
		for(int j = 0; j < NO_OSCILLATORS; j++)
		{
			env_out = env[j].Process();
        	osc[j].SetAmp(env_out);
			osc_out += osc[j].Process();
		}
		out[i] = osc_out;
		out[i+1] = osc_out;
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	hw.SetAudioBlockSize(4);

	// Init of capacitive touch
	Mpr121I2C::Config mpr_conf;
	mpr.Init(mpr_conf);	

	// Init of LED matrix
	NeoTrellisI2C::Config neo_conf;
	neo.Init(neo_conf);
	float sample_rate = hw.AudioSampleRate();

	//Set up oscillator and envelope
	for(int i = 0; i < NO_OSCILLATORS; i++)
	{
		osc[i].Init(sample_rate);
		osc[i].SetWaveform(osc[i].WAVE_SAW);
		osc[i].SetAmp(0);
		osc[i].SetFreq(1000);
		env[i].Init(sample_rate);
		env[i].SetTime(ADENV_SEG_ATTACK, 0.01);
    	env[i].SetTime(ADENV_SEG_DECAY, 0.05);
		env[i].SetMin(0.0);
    	env[i].SetMax(0.2);
    	env[i].SetCurve(0); // linear
	}

	// Buttons used to change the octave 
	Switch buttons[4];
	for(int i = 0; i < 4; i++)
	{
		buttons[i].Init(hw.GetPin(i + 1), 1000);
	}

	hw.StartAudio(AudioCallback);
	
	while(1) {
		uint16_t touched = mpr.Touched();
		g_MPR121_state = touched;

		for(int i = 0; i < 4; i++)
		{
			buttons[i].Debounce();

			if(buttons[i].Pressed())
			{
				octave = i;
			}
		}
		for(int i = 0; i < NO_OSCILLATORS + NO_OCTAVES; i++)
		{
			if ((touched >> i) & 0x01)
			{
				uint32_t col_arr[3] = {0, 0, 0};
				if(octave == 0)
				{
					col_arr[0] = colors[octave][0];
					col_arr[1] = colors[octave][1] + i * 2;
					col_arr[2] = colors[octave][2] + i * 3;
				}
				if(octave == 1)
				{
					col_arr[0] = colors[octave][0] + i * 2;
					col_arr[1] = colors[octave][1];
					col_arr[2] = colors[octave][2] + i * 3;
				}
				if(octave == 2)
				{
					col_arr[0] = colors[octave][0] + i * 3;
					col_arr[1] = colors[octave][1] + i * 2;
					col_arr[2] = colors[octave][2];
				}
				if(octave == 3)
				{
					col_arr[0] = colors[octave][0] - i * 20;
					col_arr[1] = colors[octave][1] - i * 5;
					col_arr[2] = colors[octave][2] + i * 10;
				}
				neo.pixels.SetPixelColor(i, col_arr[0], col_arr[1], col_arr[2]);
				neo.pixels.Show();
				System::Delay(1);
			}
			else if(i == octave + 12)
			{
				neo.pixels.SetPixelColor(octave + 12, colors[octave][0], colors[octave][1], colors[octave][2]);
				neo.pixels.Show();
				System::Delay(1);
			}
			else
			{
				neo.pixels.SetPixelColor(i, 0x000000);
				neo.pixels.Show();
				System::Delay(1);
			}
		}
		System::Delay(1);
	}
	return 0;
}