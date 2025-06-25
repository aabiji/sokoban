#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    vec3 baseColor = texture(texture0, fragTexCoord).rgb;

    vec3 light1 = normalize(vec3(-0.5, 1.0, -0.5)); // main light from top-left
    vec3 light2 = normalize(vec3(0.3, 0.6, 0.5));   // fill light from front-right

    vec3 normal = normalize(fragNormal);

    float diff1 = max(dot(normal, light1), 0.0);
    float diff2 = max(dot(normal, light2), 0.0);

    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0)); // view direction along z-axis
    vec3 reflectDir1 = reflect(-light1, normal);
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), 28.0);
    vec3 specular = vec3(0.9) * spec1 * 0.018; 

    // very subtle fresnel for edge highlights, slightly brighter
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
    vec3 fresnelSpec = vec3(0.8) * fresnel * 0.012;

    vec3 ambientColor = vec3(0.75, 0.80, 0.85) * 0.85;
    vec3 diffuseColor = vec3(0.78, 0.83, 0.88) * 0.85;
    vec3 ambient = ambientColor * 1.1;
    vec3 diffuse = diffuseColor * (0.35 * diff1 + 0.18 * diff2);

    // reduced height-based lighting for smoother depth
    float heightFactor = clamp(fragPosition.y * 0.5 + 0.7, 0.85, 1.0);
    float ambientOcclusion = clamp(0.7 + 0.2 * normal.y * heightFactor, 0.9, 1.0);

    // very gentle contact shadow
    float contactShadow = clamp(1.0 - fragPosition.y * 0.05, 0.97, 1.0);
    ambientOcclusion *= contactShadow;

    vec3 color = baseColor * (ambient + diffuse + specular) * ambientOcclusion;
    finalColor = vec4(color, 1.0);
}
