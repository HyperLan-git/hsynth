#version 330
out vec4 fragColor;
in vec2 pixelPos;
in float pixelAlpha;

// Code
uniform mat4 padding;
uniform float time;

vec3 hsl2rgb(vec3 hsl);

void main()
{
    float pi = 3.141596;
    float t = pi*time/4.0;
    vec2 uv = vec2(pixelPos.x/900.0, pixelPos.y/700);
    float r = 0.8*(cos(5.0*(time+uv.x*uv.y))/2.0+0.5)
        + 0.3*(sin(4.0*(t-((uv.y+1.0)/(1.01+cos(t/5.0))-uv.y/3.0-0.5)-uv.y*0.3))/2.0+0.5),
        g = (cos(5.0*
            (t + uv.y*atan(4.0*uv.x*cos(t/5.0))/2.0 + 
            uv.x-uv.y*1.3))/2.0+0.5),
        b = 0.5*(cos(5.0*(t - uv.x))/2.0+0.5)
            + 0.5*(sin(4.0*(
            3.0+t-atan(
            (uv.y+20.0)/(1.01+cos(3.0+t/5.0))-uv.x/4.0-0.5)-uv.x*0.3
            ))/2.0+0.5);
    vec3 col = vec3(abs(r),
            abs(g*0.9+0.1*g/(b+0.9)), abs(0.9*b+0.2*b/(r+0.9)));
            float r2 = ((0.3+cos(floor(0.5+uv.y*8.0)*(cos(t/10.0)) + t + uv.x*5.0)+1.0)/4.0)*(1.0+cos(uv.y*pi*16.0)),
        g2 = (1.0+cos(cos(t+(uv.x-0.5)*(uv.y-0.5)*30.0)+uv.x*5.0+t*2.0))/2.0,
        b2 = mod(time+0.5 + uv.x*cos(uv.y*10.0), 1.0);
    
    vec3 col2 = vec3((1.0-b)/(r2*(1.0-g)+0.1), g2, b2);
    
    float h = cos(cos(t+(uv.x+0.5))*uv.y*pi/2.0+pi);
    float s = cos(pow((uv.y-0.5)+6.0+time/3.0, 1.5))/1.5;
    float l = (0.3+0.1*(1.0+sin(uv.x*pi*4.0+t))+0.1*(1.0+sin(uv.y*pi*4.0-t)))/1.5;
    
    vec3 col3 = hsl2rgb(vec3(h, s, l))-vec3(mod(h*r/2.0, 1.0)/4.0, atan(g*pi)/5.0, cos(time)/2.0);

    float x1 = smoothstep(0.0, 0.5, time)-smoothstep(10.0, 11.0, time)
                +smoothstep(50.0, 51.0, time)-smoothstep(60.0, 61.0, time);
    float x2 = smoothstep(10.0, 11.0, time)-smoothstep(20.0, 21.0, time)
                +smoothstep(30.0, 31.0, time)-smoothstep(40.0, 41.0, time)
                +smoothstep(70.0, 71.0, time)-smoothstep(80.0, 81.0, time)
                +smoothstep(90.0, 91.0, time)-smoothstep(99.5, 100.0, time);
    float x3 = smoothstep(20.0, 21.0, time)-smoothstep(30.0, 31.0, time)
                +smoothstep(40.0, 41.0, time)-smoothstep(50.0, 51.0, time)
                +smoothstep(60.0, 61.0, time)-smoothstep(70.0, 71.0, time)
                +smoothstep(80.0, 81.0, time)-smoothstep(90.0, 91.0, time);
    
    vec4 finalCol = vec4(col*x1+col2*x2+col3*x3,1.0);
    finalCol.r = abs(finalCol.r);
    finalCol.g = abs(finalCol.g);
    finalCol.b = abs(finalCol.b);
    fragColor = pixelAlpha * finalCol;
}

float hue2rgb(float f1, float f2, float hue) {
    if (hue < 0.0)
        hue += 1.0;
    else if (hue > 1.0)
        hue -= 1.0;
    float res;
    if ((6.0 * hue) < 1.0)
        res = f1 + (f2 - f1) * 6.0 * hue;
    else if ((2.0 * hue) < 1.0)
        res = f2;
    else if ((3.0 * hue) < 2.0)
        res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
    else
        res = f1;
    return res;
}

vec3 hsl2rgb(vec3 hsl) {
    vec3 rgb;
    
    if (hsl.y == 0.0) {
        rgb = vec3(hsl.z); // Luminance
    } else {
        float f2;
        
        if (hsl.z < 0.5)
            f2 = hsl.z * (1.0 + hsl.y);
        else
            f2 = hsl.z + hsl.y - hsl.y * hsl.z;
            
        float f1 = 2.0 * hsl.z - f2;
        
        rgb.r = hue2rgb(f1, f2, hsl.x + (1.0/3.0));
        rgb.g = hue2rgb(f1, f2, hsl.x);
        rgb.b = hue2rgb(f1, f2, hsl.x - (1.0/3.0));
    }   
    return rgb;
}
