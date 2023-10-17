#include "audio/wave.hpp"
int arrays[88][analyze_audio_max_length] = {};

struct arrays_initializer {
  arrays_initializer(){
    Wave wave_data;
    char buf[64];
    for(int i = 0; i < 44; i++){
      sprintf(buf, "audio/JKspeech/J%02d.wav", i+1);
      read_audio(wave_data, buf);
      change_sampling_hz(wave_data, analyze_sampling_hz);
      assert(wave_data.L <= analyze_audio_max_length);
      for(int j = 0; j < wave_data.L; j++) arrays[i][j] = wave_data[j];
      if(wave_data.L < analyze_audio_max_length) arrays[i][wave_data.L] = (unsigned int)-1 >> 1;
    }
    for(int i = 44; i < 88; i++){
      sprintf(buf, "audio/JKspeech/E%02d.wav", i-44+1);
      read_audio(wave_data, buf);
      change_sampling_hz(wave_data, analyze_sampling_hz);
      assert(wave_data.L <= analyze_audio_max_length);
      for(int j = 0; j < wave_data.L; j++) arrays[i][j] = wave_data[j];
      if(wave_data.L < analyze_audio_max_length) arrays[i][wave_data.L] = (unsigned int)-1 >> 1;
    }
  }
} arrays_initializer_instance;