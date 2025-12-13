/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== bg.frag ==================
static const unsigned char temp_binary_data_0[] =
"#version 330\r\n"
"out vec4 fragColor;\r\n"
"in vec2 pixelPos;\r\n"
"in float pixelAlpha;\r\n"
"\r\n"
"// Code\r\n"
"uniform mat4 padding;\r\n"
"uniform float time;\r\n"
"\r\n"
"vec3 hsl2rgb(vec3 hsl);\r\n"
"\r\n"
"void main()\r\n"
"{\r\n"
"    float pi = 3.141596;\r\n"
"    float t = pi*time/4.0;\r\n"
"    vec2 uv = vec2(pixelPos.x/900.0, pixelPos.y/700);\r\n"
"    float r = 0.8*(cos(5.0*(time+uv.x*uv.y))/2.0+0.5)\r\n"
"        + 0.3*(sin(4.0*(t-((uv.y+1.0)/(1.01+cos(t/5.0))-uv.y/3.0-0.5)-uv.y*0.3))/2.0+0.5),\r\n"
"        g = (cos(5.0*\r\n"
"            (t + uv.y*atan(4.0*uv.x*cos(t/5.0))/2.0 + \r\n"
"            uv.x-uv.y*1.3))/2.0+0.5),\r\n"
"        b = 0.5*(cos(5.0*(t - uv.x))/2.0+0.5)\r\n"
"            + 0.5*(sin(4.0*(\r\n"
"            3.0+t-atan(\r\n"
"            (uv.y+20.0)/(1.01+cos(3.0+t/5.0))-uv.x/4.0-0.5)-uv.x*0.3\r\n"
"            ))/2.0+0.5);\r\n"
"    vec3 col = vec3(abs(r),\r\n"
"            abs(g*0.9+0.1*g/(b+0.9)), abs(0.9*b+0.2*b/(r+0.9)));\r\n"
"            float r2 = ((0.3+cos(floor(0.5+uv.y*8.0)*(cos(t/10.0)) + t + uv.x*5.0)+1.0)/4.0)*(1.0+cos(uv.y*pi*16.0)),\r\n"
"        g2 = (1.0+cos(cos(t+(uv.x-0.5)*(uv.y-0.5)*30.0)+uv.x*5.0+t*2.0))/2.0,\r\n"
"        b2 = mod(time+0.5 + uv.x*cos(uv.y*10.0), 1.0);\r\n"
"    \r\n"
"    vec3 col2 = vec3((1.0-b)/(r2*(1.0-g)+0.1), g2, b2);\r\n"
"    \r\n"
"    float h = cos(cos(t+(uv.x+0.5))*uv.y*pi/2.0+pi);\r\n"
"    float s = cos(pow((uv.y-0.5)+6.0+time/3.0, 1.5))/1.5;\r\n"
"    float l = (0.3+0.1*(1.0+sin(uv.x*pi*4.0+t))+0.1*(1.0+sin(uv.y*pi*4.0-t)))/1.5;\r\n"
"    \r\n"
"    vec3 col3 = hsl2rgb(vec3(h, s, l))-vec3(mod(h*r/2.0, 1.0)/4.0, atan(g*pi)/5.0, cos(time)/2.0);\r\n"
"\r\n"
"    float x1 = smoothstep(0.0, 0.5, time)-smoothstep(10.0, 11.0, time)\r\n"
"                +smoothstep(50.0, 51.0, time)-smoothstep(60.0, 61.0, time);\r\n"
"    float x2 = smoothstep(10.0, 11.0, time)-smoothstep(20.0, 21.0, time)\r\n"
"                +smoothstep(30.0, 31.0, time)-smoothstep(40.0, 41.0, time)\r\n"
"                +smoothstep(70.0, 71.0, time)-smoothstep(80.0, 81.0, time)\r\n"
"                +smoothstep(90.0, 91.0, time)-smoothstep(99.5, 100.0, time);\r\n"
"    float x3 = smoothstep(20.0, 21.0, time)-smoothstep(30.0, 31.0, time)\r\n"
"                +smoothstep(40.0, 41.0, time)-smoothstep(50.0, 51.0, time)\r\n"
"                +smoothstep(60.0, 61.0, time)-smoothstep(70.0, 71.0, time)\r\n"
"                +smoothstep(80.0, 81.0, time)-smoothstep(90.0, 91.0, time);\r\n"
"    \r\n"
"    vec4 finalCol = vec4(col*x1+col2*x2+col3*x3,1.0);\r\n"
"    finalCol.r = abs(finalCol.r);\r\n"
"    finalCol.g = abs(finalCol.g);\r\n"
"    finalCol.b = abs(finalCol.b);\r\n"
"    fragColor = pixelAlpha * finalCol;\r\n"
"}\r\n"
"\r\n"
"float hue2rgb(float f1, float f2, float hue) {\r\n"
"    if (hue < 0.0)\r\n"
"        hue += 1.0;\r\n"
"    else if (hue > 1.0)\r\n"
"        hue -= 1.0;\r\n"
"    float res;\r\n"
"    if ((6.0 * hue) < 1.0)\r\n"
"        res = f1 + (f2 - f1) * 6.0 * hue;\r\n"
"    else if ((2.0 * hue) < 1.0)\r\n"
"        res = f2;\r\n"
"    else if ((3.0 * hue) < 2.0)\r\n"
"        res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;\r\n"
"    else\r\n"
"        res = f1;\r\n"
"    return res;\r\n"
"}\r\n"
"\r\n"
"vec3 hsl2rgb(vec3 hsl) {\r\n"
"    vec3 rgb;\r\n"
"    \r\n"
"    if (hsl.y == 0.0) {\r\n"
"        rgb = vec3(hsl.z); // Luminance\r\n"
"    } else {\r\n"
"        float f2;\r\n"
"        \r\n"
"        if (hsl.z < 0.5)\r\n"
"            f2 = hsl.z * (1.0 + hsl.y);\r\n"
"        else\r\n"
"            f2 = hsl.z + hsl.y - hsl.y * hsl.z;\r\n"
"            \r\n"
"        float f1 = 2.0 * hsl.z - f2;\r\n"
"        \r\n"
"        rgb.r = hue2rgb(f1, f2, hsl.x + (1.0/3.0));\r\n"
"        rgb.g = hue2rgb(f1, f2, hsl.x);\r\n"
"        rgb.b = hue2rgb(f1, f2, hsl.x - (1.0/3.0));\r\n"
"    }   \r\n"
"    return rgb;\r\n"
"}\r\n";

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
        case 0xf550a5ac:  numBytes = 3380; return bg_frag;
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
