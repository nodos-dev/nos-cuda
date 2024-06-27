// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#version 450

layout(binding = 0) uniform sampler2D Input;
layout(location = 0) out vec4 rt;
layout(location = 0) in vec2 uv;

//Taken from nodos ShaderCommon.glsl
float Linear2SRGBSingle(float Value)
{
    float Result;
    if (Value <= 0.0031308)
        Result = Value * 12.92;
    else
        Result = 1.055 * pow(Value, 1.0 / 2.4) - 0.055;
    return Result;
}

vec4 Linear2SRGB(vec4 Value)
{
    return vec4(
        Linear2SRGBSingle(Value.r),
        Linear2SRGBSingle(Value.g),
        Linear2SRGBSingle(Value.b),
        Value.a);
}

void main()
{
    vec4 linear = texture(Input, uv);
    rt = Linear2SRGB(linear);
}