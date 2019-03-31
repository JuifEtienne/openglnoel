#version 330


//lightning
uniform vec3 uDirectionalLightDir_vs;
uniform vec3 uDirectionalLightIntensity;

uniform sampler2D uGPosition;
uniform sampler2D uGNormal;
uniform sampler2D uGAmbient;
uniform sampler2D uGDiffuse;
uniform sampler2D uGlossyShininess;

//Shadow Map
uniform mat4 uDirLightViewProjMatrix;
uniform sampler2D uDirLightShadowMap;
uniform float uDirLightShadowMapBias;

out vec3 fColor;

void main()
{
	vec3 fPosition = vec3(texelFetch(uGPosition, ivec2(gl_FragCoord.xy), 0)); // Correspond a vViewSpacePosition dans le forward renderer

	//Shadow Map
	vec4 positionInDirLightScreen = uDirLightViewProjMatrix * vec4(fPosition, 1);
	vec3 positionInDirLightNDC = vec3(positionInDirLightScreen / positionInDirLightScreen.w) * 0.5 + 0.5;
	float depthBlockerInDirSpace = texture(uDirLightShadowMap, positionInDirLightNDC.xy).r;
	float dirLightVisibility = positionInDirLightNDC.z < depthBlockerInDirSpace + uDirLightShadowMapBias ? 1.0 : 0.0;

	//Lighting
	vec3 wi = normalize(uDirectionalLightDir_vs);
	vec3 wo = normalize(-fPosition);
	vec3 halfVector = (wo + wi)/2;
	vec3 N = vec3(texelFetch(uGNormal, ivec2(gl_FragCoord.xy), 0));
	vec3 Li = dirLightVisibility * uDirectionalLightIntensity;

	vec3 ka = vec3(texelFetch(uGAmbient, ivec2(gl_FragCoord.xy), 0));
	vec3 kd = vec3(texelFetch(uGDiffuse, ivec2(gl_FragCoord.xy), 0));
	vec3 ks = vec3(texelFetch(uGlossyShininess, ivec2(gl_FragCoord.xy), 0));

	float shininess = texelFetch(uGlossyShininess, ivec2(gl_FragCoord.xy), 0).w;

	fColor =  Li * (kd * max(0.f, dot(wi,N)) + Li * ks * pow( max(0.f, dot(halfVector,N)), shininess)) + ka;
}
