#pragma once
class collector;
// use root class inheritance to use a static gc pointer inside root class instead of global?
inline collector *garbage_collector = nullptr;