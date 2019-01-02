#version 330

uniform sampler2D U_MainTexture;
uniform sampler2D ShadowMap;

in vec3 FragPos;
in vec2 TexCoord;
in vec3 FragNormal;
in vec4 FragPosLightSpace;

out vec4 finalColor;

// 光源属性结构体
struct LightAttr
{
	vec3 direction;	// 方向光源
	vec3 ambient;
	vec3 diffuse;
};

uniform LightAttr light;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // 执行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 变换到[0,1]的范围
    projCoords = projCoords * 0.5 + 0.5;
    // 取得最近点的深度(使用[0,1]范围下的fragPosLight当坐标)
    float closestDepth = texture(ShadowMap, projCoords.xy).r; 
    // 取得当前片元在光源视角下的深度
    float currentDepth = projCoords.z;
    // 检查当前片元是否在阴影中
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(ShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            // 检查当前片元是否在阴影中
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    if (projCoords.z > 1.0)
    shadow = 0.0;

    return shadow;
}

void main() {
	vec3	ambient = light.ambient;
    float bias = max(0.0005 * (1.0 - dot(FragNormal, light.direction)), 0.00005);
    //float bias = 0;
	float shadow = ShadowCalculation(FragPosLightSpace, bias);  

	// 漫反射光成分 此时需要光线方向为指向光源
    vec3	lightDir = normalize(-light.direction);	// 翻转方向光源的方向
	vec3	normal = normalize(FragNormal);
	float	diffFactor = max(dot(lightDir, normal), 0.0);
	vec3	diffuse = diffFactor * light.diffuse;

	vec4 albedo = texture(U_MainTexture, TexCoord);

	vec3	result = ambient + ((1.0 - shadow) * diffuse);
	finalColor	= vec4(result , 1.0f) * albedo;

}
