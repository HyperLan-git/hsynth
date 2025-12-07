/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== bg.frag ==================
static const unsigned char temp_binary_data_0[] =
"#version 330\n"
"out vec4 fragColor;\n"
"in vec2 pixelPos;\n"
"in float pixelAlpha;\n"
"\n"
"// Code\n"
"uniform mat4 padding;\n"
"uniform float time;\n"
"\n"
"vec3 hsl2rgb(vec3 hsl);\n"
"\n"
"void main()\n"
"{\n"
"    float pi = 3.141596;\n"
"    float t = pi*time/4.0;\n"
"    vec2 uv = vec2(pixelPos.x/900.0, pixelPos.y/700);\n"
"    float r = 0.8*(cos(5.0*(time+uv.x*uv.y))/2.0+0.5)\n"
"        + 0.3*(sin(4.0*(t-((uv.y+1.0)/(1.01+cos(t/5.0))-uv.y/3.0-0.5)-uv.y*0.3))/2.0+0.5),\n"
"        g = (cos(5.0*\n"
"            (t + uv.y*atan(4.0*uv.x*cos(t/5.0))/2.0 + \n"
"            uv.x-uv.y*1.3))/2.0+0.5),\n"
"        b = 0.5*(cos(5.0*(t - uv.x))/2.0+0.5)\n"
"            + 0.5*(sin(4.0*(\n"
"            3.0+t-atan(\n"
"            (uv.y+20.0)/(1.01+cos(3.0+t/5.0))-uv.x/4.0-0.5)-uv.x*0.3\n"
"            ))/2.0+0.5);\n"
"    vec3 col = vec3(abs(r),\n"
"            abs(g*0.9+0.1*g/(b+0.9)), abs(0.9*b+0.2*b/(r+0.9)));\n"
"            float r2 = ((0.3+cos(floor(0.5+uv.y*8.0)*(cos(t/10.0)) + t + uv.x*5.0)+1.0)/4.0)*(1.0+cos(uv.y*pi*16.0)),\n"
"        g2 = (1.0+cos(cos(t+(uv.x-0.5)*(uv.y-0.5)*30.0)+uv.x*5.0+t*2.0))/2.0,\n"
"        b2 = mod(time+0.5 + uv.x*cos(uv.y*10.0), 1.0);\n"
"    \n"
"    vec3 col2 = vec3((1.0-b)/(r2*(1.0-g)+0.1), g2, b2);\n"
"    \n"
"    float h = cos(cos(t+(uv.x+0.5))*uv.y*pi/2.0+pi);\n"
"    float s = cos(pow((uv.y-0.5)+6.0+time/3.0, 1.5))/1.5;\n"
"    float l = (0.3+0.1*(1.0+sin(uv.x*pi*4.0+t))+0.1*(1.0+sin(uv.y*pi*4.0-t)))/1.5;\n"
"    \n"
"    vec3 col3 = hsl2rgb(vec3(h, s, l))-vec3(mod(h*r/2.0, 1.0)/4.0, atan(g*pi)/5.0, cos(time)/2.0);\n"
"\n"
"    float x1 = smoothstep(0.0, 0.5, time)-smoothstep(10.0, 11.0, time)\n"
"                +smoothstep(50.0, 51.0, time)-smoothstep(60.0, 61.0, time);\n"
"    float x2 = smoothstep(10.0, 11.0, time)-smoothstep(20.0, 21.0, time)\n"
"                +smoothstep(30.0, 31.0, time)-smoothstep(40.0, 41.0, time)\n"
"                +smoothstep(70.0, 71.0, time)-smoothstep(80.0, 81.0, time)\n"
"                +smoothstep(90.0, 91.0, time)-smoothstep(99.5, 100.0, time);\n"
"    float x3 = smoothstep(20.0, 21.0, time)-smoothstep(30.0, 31.0, time)\n"
"                +smoothstep(40.0, 41.0, time)-smoothstep(50.0, 51.0, time)\n"
"                +smoothstep(60.0, 61.0, time)-smoothstep(70.0, 71.0, time)\n"
"                +smoothstep(80.0, 81.0, time)-smoothstep(90.0, 91.0, time);\n"
"    \n"
"    vec4 finalCol = vec4(col*x1+col2*x2+col3*x3,1.0);\n"
"    finalCol.r = abs(finalCol.r);\n"
"    finalCol.g = abs(finalCol.g);\n"
"    finalCol.b = abs(finalCol.b);\n"
"    fragColor = pixelAlpha * finalCol;\n"
"}\n"
"\n"
"float hue2rgb(float f1, float f2, float hue) {\n"
"    if (hue < 0.0)\n"
"        hue += 1.0;\n"
"    else if (hue > 1.0)\n"
"        hue -= 1.0;\n"
"    float res;\n"
"    if ((6.0 * hue) < 1.0)\n"
"        res = f1 + (f2 - f1) * 6.0 * hue;\n"
"    else if ((2.0 * hue) < 1.0)\n"
"        res = f2;\n"
"    else if ((3.0 * hue) < 2.0)\n"
"        res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;\n"
"    else\n"
"        res = f1;\n"
"    return res;\n"
"}\n"
"\n"
"vec3 hsl2rgb(vec3 hsl) {\n"
"    vec3 rgb;\n"
"    \n"
"    if (hsl.y == 0.0) {\n"
"        rgb = vec3(hsl.z); // Luminance\n"
"    } else {\n"
"        float f2;\n"
"        \n"
"        if (hsl.z < 0.5)\n"
"            f2 = hsl.z * (1.0 + hsl.y);\n"
"        else\n"
"            f2 = hsl.z + hsl.y - hsl.y * hsl.z;\n"
"            \n"
"        float f1 = 2.0 * hsl.z - f2;\n"
"        \n"
"        rgb.r = hue2rgb(f1, f2, hsl.x + (1.0/3.0));\n"
"        rgb.g = hue2rgb(f1, f2, hsl.x);\n"
"        rgb.b = hue2rgb(f1, f2, hsl.x - (1.0/3.0));\n"
"    }   \n"
"    return rgb;\n"
"}\n";

const char* bg_frag = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0xf550a5ac:  numBytes = 3284; return bg_frag;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "bg_frag"
};

const char* originalFilenames[] =
{
    "bg.frag"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
