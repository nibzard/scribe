/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cstdint>
#include <vector>

namespace audio {

void play_tone(int frequency, double durationSec = 0.1);

void play_melody(const std::vector<int>& midiList, double durationSec = 0.1);

void play_tone_from_midi(int midi, double durationSec = 0.1);

void play_random_tone(int semitoneShift = 24, double durationSec = 0.1);

void play_next_tone_progression(double durationSec = 0.1);

void play_chord(const std::vector<int>& midiNotes, double durationSec = 0.15);

void play_random_chord(int semitoneShift = 0, double durationSec = 0.15);

void play_next_chord_progression(double durationSec = 0.15);

}  // namespace audio
