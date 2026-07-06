#pragma once

#include <glad/glad.h>
#include <cstdint>

struct Vector2u8
{
	uint8_t u = 0;
	uint8_t v = 0;
};

struct Vector2F
{
	float u = 0.0f;
	float v = 0.0f;
};

struct Vector3S
{
	int16_t x = 0;
	int16_t y = 0;
	int16_t z = 0;
};

struct Vector3I
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t z = 0;
};

struct RGBu8
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
};