# Capo Lite

### Minmalist C++20 audio library

[![Build Status](https://github.com/capo-devs/capo-lite/actions/workflows/ci.yml/badge.svg)](https://github.com/capo-devs/capo-lite/actions/workflows/ci.yml)

#### Features

- Audio source 3D positioning
- Audio clip playback (direct)
- Streaming playback with accurate and near-instant seeking
- RAII types
- Exception-less implementation
- Error callback (optional)

#### Requirements

- C++20 compiler (and standard library)
- CMake 3.18+

#### Supported Formats

- WAV
- FLAC
- MP3

#### Usage

```cpp
#include <capo/capo.hpp>

void capo_test() {
  auto device = capo::Device{};
  auto [sound_pcm, _] = capo::Pcm::from_file("audio_clip.wav"); // load / decompress audio file into Pcm
  auto sound_source = device.make_sound_source(); // make a new Sound Source instance
  sound_source.set_clip(sound_pcm.clip()); // set clip on Sound Source
  sound_source.play(); // play sound
  while (sound_source.state() == capo::State::ePlaying) {
    std::this_thread::yield(); // wait until playback complete
  }
  auto [music_pcm, _] = capo::Pcm::from_file("music_file.mp3"); // load / decompress audio file into Pcm
  auto stream_source = device.make_stream_source(); // make a new Stream Source instance
  stream_source.set_stream(music_pcm.clip()); // set clip on Stream Source; source Pcm must outlive stream!
  stream_source.play(); // start playback
  while (stream_source.state() == capo::State::ePlaying) {
    std::this_thread::yield(); // wait until playback complete
  }
}
```

[sound](examples/sound.cpp) and [music](examples/music.cpp) demonstrate basic sound and music usage, [interactive](examples/interactive.cpp) demonstrates a scrappy console based interactive music player.

#### Dependencies

- [OpenAL Soft](https://github.com/kcat/openal-soft)
- [dr_libs](https://github.com/mackron/dr_libs)

#### Misc

[Original repository](https://github.com/capo-devs/capo-lite)

[LICENCE](LICENSE)
