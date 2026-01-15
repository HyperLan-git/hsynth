# HSynth
A synthesizer that can parse a mathematical expression to turn it into a multidimensional wavetable. Look at [the manual](./manual/Manual.md) for more info.

## Dependencies
On windows, you just need Visual Studio 2022 and Juce.
In addition to juce's main dependencies on linux, you'll need libasan to compile in debug mode.

## How to compile
On windows, simply open the project from projucer and build the solution you need.

On linux, use the makefile :
```sh
make
```

To compile in Release mode (with optimisations and no memory sanitizer), use `make CONFIG=Release`.
You can clean binaries with `make clean`.

## Presets
```
sine: sin(p)
tri: abs(2-abs(1-t*4))-1
square: sign(t-0.5)
saw: (t+0.5)%1*2-1
normalised sinc: ((2/(1.217234))*(sinc((p-P)*(5+a*50))+0.217234)-1)
fm saw: ((t+(b*10*(abs(2-abs(1-t*4))-1))+a*5*sin(p)+0.5)%1*2-1)*sin(p)*sign(t-0.5)
distorted sine: arctan((1+b*20)*sin(p+a*30*sin(p)))*2/P
F pulse: arctan(5*cos(20*a*a*p)*(tan(p**(1+b))))/P
F pulse 2: arctan((1+sin(t*((0.5*b+P/4)**(p*P))))**sin(p**((0.4+a*2)*p/5)))/P-0.25
P mod: sin(tan(sin(p*(1+b))+abs(sin(p*(1+b))%((a+0.2)/2+0.001))**3))
thing: arctan(abs((abs(2-abs(1-(((t+sin(p)*a*2)%1)*4)))-1))**(arctan(sin(t)**sin(t*b*300))))/P-0.125
6: arctan(gamma(t-8+t*(a*12+1)))/P
```