#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;

out vec4 fColor;

uniform vec3 viewPosEye;
uniform vec3 lightPosEye;
uniform vec3 lightColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform float constant = 1.0f;
uniform float linear = 0.09f;
uniform float quadratic = 0.032f;
uniform float shininess = 32.0f;

uniform vec3 dirLightDirEye;
uniform vec3 dirLightColor;

uniform sampler2D shadowMap;
in vec4 FragPosLightSpace;

void computeLightComponents(out vec3 ambient, out vec3 diffuse, out vec3 specular) {
    vec3 normalEye = normalize(fNormal);
    vec3 lightDirN = normalize(lightPosEye - fPosEye.xyz);
    vec3 viewDirN = normalize(viewPosEye - fPosEye.xyz);
    vec3 halfVector = normalize(lightDirN + viewDirN);

    float dist = length(lightPosEye - fPosEye.xyz);
    float att = 1.0 / (constant + linear * dist + quadratic * (dist * dist));

    vec3 diffuseMap = texture(diffuseTexture, fTexCoords).rgb;
    vec3 specularMap = texture(specularTexture, fTexCoords).rgb;

    float ambientStrength = 0.2f;
    ambient = att * ambientStrength * lightColor * diffuseMap;
    diffuse = att * max(dot(normalEye, lightDirN), 0.0f) * lightColor * diffuseMap;

    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
    specular = att * specCoeff * lightColor * specularMap;
}

void computeDirectionalLightComponents(out vec3 ambDir, out vec3 diffDir, out vec3 specDir) {
    vec3 normalEye = normalize(fNormal);
    vec3 lightDirN = normalize(dirLightDirEye);

    vec3 diffuseMap = texture(diffuseTexture, fTexCoords).rgb;
    vec3 specularMap = texture(specularTexture, fTexCoords).rgb;

    float ambientStrength = 0.1f;
    ambDir = ambientStrength * dirLightColor * diffuseMap;

    float diffFactor = max(dot(normalEye, lightDirN), 0.0f);
    diffDir = diffFactor * dirLightColor * diffuseMap;

    vec3 viewDirN = normalize(viewPosEye - fPosEye.xyz);
    vec3 halfVector = normalize(lightDirN + viewDirN);
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
    specDir = specCoeff * dirLightColor * specularMap;
}

float computeShadow(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDirEye)
{
    // Transform from clip space back to [0,1] UV
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // If outside the light’s frustum
    if (projCoords.z > 1.0) 
        return 0.0;

    // Read closest depth from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Simple dynamic bias based on angle
    float bias = max(0.001 * (1.0 - dot(normal, lightDirEye)), 0.0005);

    // For PCF, sample a 3x3 region
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}


void main() {
    float shadowAmount = computeShadow(shadowMap, FragPosLightSpace, normalize(fNormal), normalize(dirLightDirEye));
   vec3 ambientP, diffuseP, specularP;
    computeLightComponents(ambientP, diffuseP, specularP);

    vec3 ambDir, diffDir, specDir;
    computeDirectionalLightComponents(ambDir, diffDir, specDir);



    vec3 lighting =  (ambDir + (1.0 - shadowAmount ) * (diffDir + specDir))+ (ambientP + (1.0 - shadowAmount) * (diffuseP + specularP));

    fColor = vec4(clamp(lighting, 0.0, 1.0), 1.0);
}
