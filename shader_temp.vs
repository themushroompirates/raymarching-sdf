#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

uniform vec3 viewEye;
uniform vec3 viewCenter;
uniform float runTime;
uniform vec2 resolution;

uniform float thingy = 2.0;
uniform vec3 cameraUp = vec3(0., 1., 0.);

uniform mat4 modelViewMatrix;
uniform mat4 modelProjMatrix;

vec3 colDiffuse = vec3(.2, .4, .6);

// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// A list of useful distance function to simple primitives, and an example on how to
// do some interesting boolean operations, repetition and displacement.
//
// More info here: http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm

#define AA 2   // make this 1 is your machine is too slow

//------------------------------------------------------------------

float length2( vec2 p )
{
    return sqrt( p.x*p.x + p.y*p.y );
}

float length6( vec2 p ) { p = p*p*p; p = p*p; return pow( p.x + p.y, 1.0/6.0 ); }
float length6( vec3 p ) { p = p*p*p; p = p*p; return pow( p.x + p.y + p.z, 1.0/6.0 ); }

float length8( vec2 p )
{
    p = p*p; p = p*p; p = p*p;
    return pow( p.x + p.y, 1.0/8.0 );
}

float sdPlane( vec3 p )
{
    return p.y;
}

float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}

float sdBox( vec3 p, vec3 b )
{
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdEllipsoid( in vec3 p, in vec3 r )
{
    return (length( p/r ) - 1.0) * min(min(r.x,r.y),r.z);
}

float udRoundBox( vec3 p, vec3 b, float r )
{
    return length(max(abs(p)-b,0.0))-r;
}

float sdTorus( vec3 p, vec2 t )
{
    return length( vec2(length(p.xz)-t.x,p.y) )-t.y;
}

float sdHexPrism( vec3 p, vec2 h )
{
    vec3 q = abs(p);
#if 0
    return max(q.z-h.y,max((q.x*0.866025+q.y*0.5),q.y)-h.x);
#else
    float d1 = q.z-h.y;
    float d2 = max((q.x*0.866025+q.y*0.5),q.y)-h.x;
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
#endif
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
    vec3 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float sdEquilateralTriangle(  in vec2 p )
{
    const float k = sqrt(3.0);
    p.x = abs(p.x) - 1.0;
    p.y = p.y + 1.0/k;
    if( p.x + k*p.y > 0.0 ) p = vec2( p.x - k*p.y, -k*p.x - p.y )/2.0;
    p.x += 2.0 - 2.0*clamp( (p.x+2.0)/2.0, 0.0, 1.0 );
    return -length(p)*sign(p.y);
}

float sdTriPrism( vec3 p, vec2 h )
{
    vec3 q = abs(p);
    float d1 = q.z-h.y;
#if 1
    // distance bound
    float d2 = max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5;
#else
    // correct distance
    h.x *= 0.866025;
    float d2 = sdEquilateralTriangle(p.xy/h.x)*h.x;
#endif
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
}

float sdCylinder( vec3 p, vec2 h )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdCappedCylinder( vec3 p, vec3 a, vec3 b, float r )
{
  vec3  ba = b - a;
  vec3  pa = p - a;
  float baba = dot(ba,ba);
  float paba = dot(pa,ba);
  float x = length(pa*baba-ba*paba) - r*baba;
  float y = abs(paba-baba*0.5)-baba*0.5;
  float x2 = x*x;
  float y2 = y*y*baba;
  float d = (max(x,y)<0.0)?-min(x2,y2):(((x>0.0)?x2:0.0)+((y>0.0)?y2:0.0));
  return sign(d)*sqrt(abs(d))/baba;
}

float sdCappedCone( vec3 p, vec3 a, vec3 b, float ra, float rb )
{
  float rba  = rb-ra;
  float baba = dot(b-a,b-a);
  float papa = dot(p-a,p-a);
  float paba = dot(p-a,b-a)/baba;
  float x = sqrt( papa - paba*paba*baba );
  float cax = max(0.0,x-((paba<0.5)?ra:rb));
  float cay = abs(paba-0.5)-0.5;
  float k = rba*rba + baba;
  float f = clamp( (rba*(x-ra)+paba*baba)/k, 0.0, 1.0 );
  float cbx = x-ra - f*rba;
  float cby = paba - f;
  float s = (cbx<0.0 && cay<0.0) ? -1.0 : 1.0;
  return s*sqrt( min(cax*cax + cay*cay*baba,
                     cbx*cbx + cby*cby*baba) );
}

float sdCone( in vec3 p, in vec3 c )
{
    vec2 q = vec2( length(p.xz), p.y );
    float d1 = -q.y-c.z;
    float d2 = max( dot(q,c.xy), q.y);
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
}

float sdConeSection( in vec3 p, in float h, in float r1, in float r2 )
{
    float d1 = -p.y - h;
    float q = p.y - h;
    float si = 0.5*(r1-r2)/h;
    float d2 = max( sqrt( dot(p.xz,p.xz)*(1.0-si*si)) + q*si - r2, q );
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
}

float sdPryamid4(vec3 p, vec3 h ) // h = { cos a, sin a, height }
{
    // Tetrahedron = Octahedron - Cube
    float box = sdBox( p - vec3(0,-2.0*h.z,0), vec3(2.0*h.z) );

    float d = 0.0;
    d = max( d, abs( dot(p, vec3( -h.x, h.y, 0 )) ));
    d = max( d, abs( dot(p, vec3(  h.x, h.y, 0 )) ));
    d = max( d, abs( dot(p, vec3(  0, h.y, h.x )) ));
    d = max( d, abs( dot(p, vec3(  0, h.y,-h.x )) ));
    float octa = d - h.z;
    return max(-box,octa); // Subtraction
 }


float sdTorus82( vec3 p, vec2 t )
{
    vec2 q = vec2(length2(p.xz)-t.x,p.y);
    return length8(q)-t.y;
}

float sdTorus88( vec3 p, vec2 t )
{
    vec2 q = vec2(length8(p.xz)-t.x,p.y);
    return length8(q)-t.y;
}

float sdCylinder6( vec3 p, vec2 h )
{
    return max( length6(p.xz)-h.x, abs(p.y)-h.y );
}

//------------------------------------------------------------------

float opS( float d1, float d2 )
{
    return max(-d2,d1);
}

float opU( float d1, float d2 )
{
    return min(d1, d2);
}

float opI( float d1, float d2 )
{
    return max(d1, d2);
}


float opSmoothUnion( float k, float d1, float d2 )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

// @NOTE swapping the arguments for consistency with opS
float opSmoothSubtraction( float k, float d2, float d1 )
{
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h);
}

float opSmoothIntersection( float k, float d1, float d2 )
{
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h);
}

float opSmooth(float k, float d) {
    return d-k;
}

vec3 opRep( vec3 p, vec3 c )
{
    return mod(p,c)-0.5*c;
}

vec3 opTwist( vec3 p )
{
    float  c = cos(10.0*p.y+10.0);
    float  s = sin(10.0*p.y+10.0);
    mat2   m = mat2(c,-s,s,c);
    return vec3(m*p.xz,p.y);
}

vec2 addObject( vec2 d1, vec2 d2 )
{
    return (d1.y<d2.y) ? d1 : d2;
}

//------------------------------------------------------------------

vec2 map( in vec3 pos );

vec2 castRay( in vec3 ro, in vec3 rd )
{
    float tmin = 0.2;
    float tmax = 30.0;

#if 0
    // bounding volume ()
    float yMin = -20.0, yMax = 20.0;
    float tp1 = (yMin-ro.y)/rd.y; if( tp1>0.0 ) tmax = min( tmax, tp1 );
    float tp2 = (yMax-ro.y)/rd.y; if( tp2>0.0 ) { if( ro.y>1.6 ) tmin = max( tmin, tp2 );
                                                 else           tmax = min( tmax, tp2 ); }
#endif

    float t = tmin;
    float m = -1.0;
    for( int i=0; i<64; i++ )
    {
        float precis = 0.0005*t;
        vec2 res = map( ro+rd*t );
        if( res.y<precis || t>tmax ) break;
        t += res.y;
        m = res.x;
    }

    if( t>tmax ) m=-1.0;
    return vec2( m, t );
}


float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
    float res = 1.0;
    float t = mint;
    for( int i=0; i<16; i++ )
    {
        float h = map( ro + rd*t ).y;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
}

vec3 calcNormal( in vec3 pos )
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize( e.xyy*map( pos + e.xyy ).y +
                      e.yyx*map( pos + e.yyx ).y +
                      e.yxy*map( pos + e.yxy ).y +
                      e.xxx*map( pos + e.xxx ).y );
    /*
    vec3 eps = vec3( 0.0005, 0.0, 0.0 );
    vec3 nor = vec3(
        map(pos+eps.xyy).y - map(pos-eps.xyy).y,
        map(pos+eps.yxy).y - map(pos-eps.yxy).y,
        map(pos+eps.yyx).y - map(pos-eps.yyx).y );
    return normalize(nor);
    */
}

float calcAO( in vec3 pos, in vec3 nor )
{
    float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos ).y;
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );
}

// http://iquilezles.org/www/articles/checkerfiltering/checkerfiltering.htm
float checkersGradBox( in vec2 p )
{
    // filter kernel
    vec2 w = fwidth(p) + 0.001;
    // analytical integral (box filter)
    vec2 i = 2.0*(abs(fract((p-0.5*w)*0.5)-0.5)-abs(fract((p+0.5*w)*0.5)-0.5))/w;
    // xor pattern
    return 0.5 - 0.5*i.x*i.y;
}

vec4 render( in vec3 ro, in vec3 rd )
{
    vec3 col = vec3(0.7, 0.9, 1.0) +rd.y*0.8;
    vec2 res = castRay(ro,rd);
    float m = res.x;
    float t = res.y;
    
    float depth = 0.0f;
    
    if( m>-0.5 )
    {
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal( pos );
        vec3 ref = reflect( rd, nor );
        
        depth = t;

        // material
        //col = vec3(.8, .6, .2);
        //col = vec3(.6, .1, .6);
        col = colDiffuse;

        // lighting
        float occ = calcAO( pos, nor );
        vec3  lig = normalize( vec3(cos(-0.4 * (runTime*0.+1.)), sin(0.7 * (runTime*0.+1.)), -0.6) );
        vec3  hal = normalize( lig-rd );
        float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );
        float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
        float bac = clamp( dot( nor, normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 )*clamp( 1.0-pos.y,0.0,1.0);
        float dom = smoothstep( -0.1, 0.1, ref.y );
        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );

        dif *= calcSoftshadow( pos, lig, 0.02, 2.5 );
        dom *= calcSoftshadow( pos, ref, 0.02, 2.5 );

        float spe = pow( clamp( dot( nor, hal ), 0.0, 1.0 ),16.0)*
                    dif *
                    (0.04 + 0.96*pow( clamp(1.0+dot(hal,rd),0.0,1.0), 5.0 ));

        vec3 lin = vec3(0.0);
        lin += 1.30*dif*vec3(1.00,0.80,0.55);
        lin += 0.40*amb*vec3(0.40,0.60,1.00)*occ;
        lin += 0.50*dom*vec3(0.40,0.60,1.00)*occ;
        lin += 0.50*bac*vec3(0.25,0.25,0.25)*occ;
        lin += 0.25*fre*vec3(1.00,1.00,1.00)*occ;
        col = col*lin;
        //col += 10.00*spe*vec3(1.00,0.90,0.70);
        col = mix(col, col + 10.00*spe*vec3(1.00,0.90,0.70), .2);

        //col = mix( col, vec3(0.8,0.9,1.0), 1.0-exp( -0.0002*t*t*t ) );
    }
    else {
        //discard;
        return vec4(1.,0.,1.,0.);
    }

    return vec4( clamp(col,0.0,1.0), depth );
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = cameraUp; //vec3(sin(cr), cos(cr),0.0);
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

void main()
{
    vec4 tot = vec4(0.0);
    float totCoverage = 0.;
    float divisor = 0.;
    
#if AA>1
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
        // pixel coordinates
        vec2 o = vec2(float(m),float(n)) / float(AA) - 0.5;
        vec2 p = (-resolution.xy + 2.0*(gl_FragCoord.xy+o))/resolution.y;
#else
        vec2 p = (-resolution.xy + 2.0*gl_FragCoord.xy)/resolution.y;
#endif

        // RAY: Camera is provided from raylib
        //vec3 ro = vec3( -0.5+3.5*cos(0.1*time + 6.0*mo.x), 1.0 + 2.0*mo.y, 0.5 + 4.0*sin(0.1*time + 6.0*mo.x) );

        vec3 ro = viewEye;
        vec3 ta = viewCenter;

        // camera-to-world transformation
        mat3 ca = setCamera( ro, ta, 0.0 );
        // ray direction
        vec3 rd = ca * normalize( vec3(p.xy, thingy) );
        //vec3 rd = ca * normalize( vec3(p.xy,atan()) );

        // render
        vec4 col = render( ro, rd );
        
        if (col.w == 0.) {
            col = vec4(0.);
        }
        else {
            vec3 pos = ro + rd * col.w;
            
            float far=gl_DepthRange.far; float near=gl_DepthRange.near;

            vec4 eye_space_pos  = modelViewMatrix * vec4(pos, 1.0);
            vec4 clip_space_pos = modelProjMatrix * eye_space_pos;

            float ndc_depth = clip_space_pos.z / clip_space_pos.w;

            float depth = (((far-near) * ndc_depth) + near + far) / 2.0;
            
            col.w = depth;
            
            divisor += 1.;
            totCoverage += 1.;

            // gamma
            col.xyz = pow( col.xyz, vec3(0.4545) );
        }

        tot += col;
#if AA>1
    }
    if (totCoverage == 0.) discard;
    
    tot /= divisor;
    gl_FragDepth = tot.w;
    
    tot.w = totCoverage / float(AA*AA);
#else
    if (totCoverage == 0.) discard;
    
    gl_FragDepth = tot.w;
    tot.w = totCoverage;
#endif

    
    //tot = mix(tot, vec4(1., 0., 1., 1.), 0.5);
    
    //tot = vec4(1., 0., 0.5, 1.);

    finalColor = vec4( tot );
}

vec2 map( in vec3 pos )
{
    vec2 res = vec2(-1.0, 9999.99);
    colDiffuse = vec3(0.000,0.620,0.184);
    res = addObject( res, vec2( 1.0, 
        opSmoothUnion(0.250, 
            /* union child 1 of 2 */
            opSmooth(0.250, 
                sdBox(pos - vec3(0.000, 0.000, 0.000), vec3(1.250, 0.250, 0.250))
            ),
            /* union child 2 of 2 */
            sdSphere(pos - vec3(1.000, 0.000, 0.000), 1.000)
        ) /* End union */
    )); /* End object */

    return res;
}