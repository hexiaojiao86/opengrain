/**    
    @file wavegrain.c
    @brief
    @author John Williamson
    
    Copyright (c) 2011 All rights reserved.
    Licensed under the BSD 3 clause license. See COPYING.
    
    This file is part of the OpenGrain distribution.
    http://opengrain.sourceforge.net   
*/              

#include "wavegrain.h"



// create a wavegrain parameter set
WaveGrainParameters *create_wave_parameters(GrainSource *source)
{
   WaveGrainParameters *wavegrain;
   wavegrain = malloc(sizeof(*wavegrain));
   wavegrain->phase = create_distribution();  
   wavegrain->pitch_shift = create_distribution();
   wavegrain->sound = NULL;    
   
   wavegrain->phase_offset = GLOBAL_STATE.elapsed_samples;
   
   wavegrain->phase_rate = 0.0;
   wavegrain->interpolate = 0;
   
   wavegrain->grain_phase = 0;
   // defaults
   set_constant_distribution(wavegrain->phase, 0.0);   
   set_constant_distribution(wavegrain->pitch_shift, 0.0);
   return wavegrain;
   
}


// destroy a wavegrain parameter set
void destroy_wave_parameters(WaveGrainParameters *wavegrain)
{
    destroy_distribution(wavegrain->phase);    
    destroy_distribution(wavegrain->pitch_shift);    
    free(wavegrain);
    
    // don't free the sound, we don't own it!
}


// get the phase distribution (this is added to the phase offset)
Distribution * get_phase_distribution_wave_parameters(WaveGrainParameters *wavegrain)
{

    return wavegrain->phase;
}


// restart the phase offset 
void restart_wave_parameters(WaveGrainParameters *wavegrain)
{
    wavegrain->phase_offset = GLOBAL_STATE.elapsed_samples; 
    wavegrain->grain_phase = 0;
}


// set the playback rate
void set_rate_wave_parameters(WaveGrainParameters *wavegrain, float rate)
{
    wavegrain->phase_rate = rate;
}


// get the pitch shift distribution (in semitones)
Distribution * get_pitch_shift_distribution_wave_parameters(WaveGrainParameters *wavegrain)
{
    return wavegrain->pitch_shift;

}

// set whether grains are locked to real time, or to the grain time
void set_phase_mode_wave_parameters(WaveGrainParameters *wavegrain, int mode)
{
    wavegrain->phase_mode = mode;
}

// enable/disable linear interpolation for pitch rates != 0.0
void set_interpolation_wave_parameters(WaveGrainParameters *wavegrain, int interpolate)
{
    wavegrain->interpolate = interpolate;
}

// set the source waveform
void set_source_wave_parameters(WaveGrainParameters *wavegrain, WaveSound *sound)
{
    wavegrain->sound = sound;

}




// create an empty wavegrain
void *create_wavegrain(void *source)
{
    WaveGrain *grain;
    WaveGrainParameters *parameters;
    parameters = (WaveGrainParameters *) source;
    grain = malloc(sizeof(*grain));
        
    return grain;
}


// initialise a wavegrain, with the appropriate phase info
void init_wavegrain(void *wgrain, void *source, Grain *grain)
{
    WaveGrain *wavegrain;
    Buffer *channel;
    WaveGrainParameters *parameters;
    int increment;
    
    parameters = (WaveGrainParameters *) source;
    wavegrain = (WaveGrain*) wgrain;
        
    if(parameters->phase_mode == WAVEGRAIN_PHASE_GRAINTIME)
    {
        // grain time
        increment = parameters->grain_phase * parameters->phase_rate;
        parameters->grain_phase += grain->duration_samples;
    }
    else
    {
        // real time
        increment = (GLOBAL_STATE.elapsed_samples - parameters->phase_offset ) *  parameters->phase_rate;
    }
           
    // set the waveform, phase, and pitch rate
    wavegrain->sound = parameters->sound;    
    
    channel = get_channel_wave_sound(wavegrain->sound, 0);
    
    wavegrain->phase = sample_from_distribution(parameters->phase) * channel->n_samples + increment;        
    wavegrain->phase = ((int) wavegrain->phase) % channel->n_samples;        
    
    wavegrain->rate = SEMITONES_TO_RATE(sample_from_distribution(parameters->pitch_shift));
    wavegrain->interpolate = parameters->interpolate;
    
}

// free a wavegrain structure
void destroy_wavegrain(void *wgrain)
{
    WaveGrain *wavegrain;
    wavegrain = (WaveGrain*) wgrain;
    free(wavegrain);        
}


// copy a waveform into a buffer, optionally with interpolation
void fill_wavegrain(void *wgrain, Buffer *buffer)
{
    int i, j;
    float fractional;
    Buffer *channel;
    WaveGrain *wavegrain;    
    wavegrain = (WaveGrain*) wgrain;
    
    channel = get_channel_wave_sound(wavegrain->sound, 0);
    
    if(wavegrain->interpolate)
    {
        for(i=0;i<buffer->n_samples;i++)
        {
            
            wavegrain->phase += wavegrain->rate;           
            
            // loop wave
            if(wavegrain->phase >= channel->n_samples)        
                wavegrain->phase -= channel->n_samples;
                    
            fractional = wavegrain->phase - floor(wavegrain->phase);
            j = (int)wavegrain->phase;
            
            if(wavegrain->phase>0)
                buffer->x[i] = (1-fractional)*channel->x[j-1] +(fractional)*channel->x[j];
                else
                buffer->x[i] = channel->x[j]; // no sample before
        
        }
    
    }
    else
    {   
        for(i=0;i<buffer->n_samples;i++)
        {
            wavegrain->phase += wavegrain->rate;           
            // loop wave
            if(wavegrain->phase >= channel->n_samples)        
                wavegrain->phase -= channel->n_samples;
            j = (int)wavegrain->phase;        
            // no interpolation!
            buffer->x[i] = channel->x[j];
        
        }
    }

}


