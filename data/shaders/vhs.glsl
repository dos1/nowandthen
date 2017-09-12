#ifdef GL_ES
precision highp float;
precision highp int;
#endif

uniform sampler2D al_tex;
varying vec2 varying_texcoord;
varying vec4 varying_color;

uniform bool autoScan;
uniform float xScanline;
uniform float xScanline2;
uniform float yScanline;
uniform float xScanlineSize;
uniform float xScanlineSize2;
uniform float yScanlineAmount;
uniform float grainLevel;
uniform float scanFollowAmount;
uniform float analogDistort;
uniform float bleedAmount;
uniform float bleedDistort;
uniform float bleedRange;
uniform vec4 colorBleedL;
uniform vec4 colorBleedC;
uniform vec4 colorBleedR;
uniform float TIME;
uniform vec2 RENDERSIZE;

//  Based on https://www.interactiveshaderformat.com/sketches/871
//	which was based on https://github.com/staffantan/unity-vhsglitch
//	and converted by David Lublin / VIDVOX

const float tau = 6.28318530718;

vec4 texture2DNorm(sampler2D tex, vec2 pos) {
	// FIXME: it's needed, but no idea why
#ifdef GL_ES
	pos.x /= 1.065;
	pos.y /= 1.895;
#endif
	return texture2D(tex, pos);
}

float rand(vec3 co){
	return abs(mod(sin( dot(co.xyz ,vec3(12.9898,78.233,45.5432) )) * 43758.5453, 1.0));
}

void main()	{
	float	actualXLine = (!autoScan) ? xScanline : mod(xScanline + ((1.0+sin(0.34*TIME))/2.0 + (1.0+sin(TIME))/3.0 + (1.0+cos(2.1*TIME))/3.0 + (1.0+cos(0.027*TIME))/2.0)/3.5,1.0);
	float	actualXLineWidth = (!autoScan) ? xScanlineSize : 2.0 * xScanlineSize * ((1.0+sin(1.2*TIME))/2.0 + (1.0+cos(3.91*TIME))/3.0 + (1.0+cos(0.014*TIME))/2.0)/3.5;
	vec2 loc = varying_texcoord;
#ifdef GL_ES
	// FIXME: it's needed, but no idea why
	loc.x *= 1.065;
	loc.y *= 1.895;
#endif
	float	dx = 1.0+actualXLineWidth/25.0-abs(distance(loc.y, actualXLine));
	float	dx2 = 1.0+xScanlineSize2/10.0-abs(distance(loc.y, xScanline2));
	float	dy = (1.0-abs(distance(loc.y, yScanline)));
	if (autoScan) {
		dy = (1.0-abs(distance(loc.y, mod(yScanline+TIME,1.0))));
	}

	dy = (dy > 0.5) ? 2.0 * dy : 2.0 * (1.0 - dy);

	float	rX = (rand(vec3(dy,actualXLine,analogDistort)) * scanFollowAmount) + (rand(vec3(dy,bleedAmount,analogDistort)) * (1.0-scanFollowAmount));
	float	xTime = (actualXLine > 0.5) ? 2.0 * actualXLine : 2.0 * (1.0 - actualXLine);

	loc.x += yScanlineAmount * dy * 0.025 + analogDistort * rX/(RENDERSIZE.x/2.0);

	if (dx2 > 1.0 - xScanlineSize2 / 10.0)	{
		float	rX2 = (dy * rand(vec3(dy,dx2,dx)) + dx2) / 4.0;
		float	distortAmount = analogDistort * (sin(rX * tau / dx2) + cos(rX * tau * 0.78 / dx2)) / 10.0;
		loc.x += (1.0 + distortAmount * sin(tau * (loc.x) / rX2 ) - 1.0) / 15.0;
	}
	if (dx > 1.0 - actualXLineWidth / 25.0) {
		loc.y = actualXLine;
	}

	loc.x = mod(loc.x,1.0);
	loc.y = mod(loc.y,1.0);

	vec4	c = texture2DNorm(al_tex, loc);
	float	x = (loc.x*320.0)/320.0;
	float	y = (loc.y*240.0)/240.0;
	float	bleed = 0.0;

	c -= (rand(vec3(x, y, xTime)) * xTime / (5.0-grainLevel)) * scanFollowAmount;
	c -= rand(vec3(x, y, bleedAmount)) * (bleedAmount/20.0) / (5.0-grainLevel) * (1.0 - scanFollowAmount);

	if (bleedAmount > 0.0)	{
		bleed += texture2DNorm(al_tex, loc + vec2(0.01, 0)).r;
		bleed += texture2DNorm(al_tex, loc + bleedRange * vec2(0.02, 0)).r;
		bleed += texture2DNorm(al_tex, loc + bleedRange * vec2(0.01, 0.01)).r;
		bleed += texture2DNorm(al_tex, loc + bleedRange * vec2(-0.02, 0.02)).r;
		bleed += texture2DNorm(al_tex, loc + bleedRange * vec2(0.0, -0.03)).r;
		bleed /= 6.0;
		bleed *= bleedAmount;
	}

	if (bleed > 0.1) {
		float	bleedFreq = 1.0;
		float	bleedX = 0.0;
		if (autoScan) {
			bleedX = x + bleedDistort * (yScanlineAmount + (1.5 + cos(TIME / 13.0 + tau*(bleedDistort+(1.0-loc.y))))/2.0) * sin((TIME / 9.0 + bleedDistort) * tau + loc.y * loc.y * tau * bleedFreq) / 8.0;
		} else {
			bleedX = x + (yScanlineAmount + (1.0 + sin(tau*(bleedDistort+loc.y)))/2.0) * sin(bleedDistort * tau + loc.y * loc.y * tau * bleedFreq) / 10.0;
		}
		vec4	colorBleed = (bleedX < 0.5) ? mix(colorBleedL, colorBleedC, 2.0 * bleedX) : mix(colorBleedR, colorBleedC, 2.0 - 2.0 * bleedX);
		c += (bleed * max(xScanlineSize,xTime) * colorBleed) * scanFollowAmount;
		c += (bleed * colorBleed) * (1.0 - scanFollowAmount);
	}

	c.rgb *= c.a;
	c.a = 1.0;
	gl_FragColor = c;
}
