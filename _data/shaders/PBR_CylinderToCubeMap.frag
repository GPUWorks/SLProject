//#############################################################################
//  File:      PBR_CylinderToCubeMap.frag
//  Purpose:   GLSL fragment program which takes fragment's direction to sample
//             the equirectangular map as if it is a cube map. Based on the
//             physically based rendering (PBR) tutorial with GLSL by Joey de
//             Vries on https://learnopengl.com/PBR/IBL/Diffuse-irradiance
//  Author:    Carlos Arauz
//  Date:      April 2018
//  Copyright: Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

varying vec3      P_VS;        // fragment direction

uniform sampler2D u_texture0;  // Equirectagular map

const   vec2      invAtan = vec2(0.1591, 0.3183);

//-----------------------------------------------------------------------------
// takes the fragment's direction and does some trigonometry to give a texture
// coordinate from an equirectagular map as it it is a cube map
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

//-----------------------------------------------------------------------------
void main()
{
    vec2 uv = SampleSphericalMap(normalize(P_VS));
    vec3 color = texture(u_texture0, uv).rgb;
    
    gl_FragColor = vec4(color, 1.0);
}
//-----------------------------------------------------------------------------
