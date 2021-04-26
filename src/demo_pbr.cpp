#include "demo_pbr.hpp"

#include <string>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"

constexpr int nrRows = 7;
constexpr int nrColumns = 7;
constexpr float spacing = 2.5;

struct Vertex
{
    float3 position;
    float2 UV;
    float3 normal;
};

DemoPBR::DemoPBR(const DemoInputs& inputs)
{
    mainCamera.position = { 0,0,4.f };

    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;
        {
            VertexDescriptor descriptor = {};
            descriptor.size = sizeof(Vertex);
            descriptor.positionOffset = offsetof(Vertex, position);
            descriptor.hasUV = true;
            descriptor.uvOffset = offsetof(Vertex, UV);
            descriptor.hasNormal = true;
            descriptor.normalOffset = offsetof(Vertex, normal);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            pbrSphere.mesh = meshBuilder.GenUVSphere(nullptr,48,64);
        }

        // In VRAM
        glGenBuffers(1, &pbrSphere.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, pbrSphere.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        //free(vertices);
    }

    {
        glGenVertexArrays(1, &pbrSphere.VAO);
        glBindVertexArray(pbrSphere.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, pbrSphere.VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, UV));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
    }

    basicPBR.id = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec3 aNormal;        

        out vec2 vUV;
        out vec3 vWorldPos;
        out vec3 vNormal;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            vUV = aUV;
            vWorldPos = vec3(model * vec4(aPosition,1.0));
            vNormal = mat3(model) * aNormal;

            gl_Position = projection * view * vec4(vWorldPos, 1.0);
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        out vec4 fragColor;

        in vec2 vUV;
        in vec3 vWorldPos;
        in vec3 vNormal;

        uniform vec3 camPos;
        
        //Material
        uniform vec3 albedo;
        uniform float metallic;
        uniform float roughness;
        uniform float ao;

        //Lights
        uniform vec3 lightPositions[4];
        uniform vec3 lightColors[4];

        const float PI = 3.14159265359;
        

        vec3 FresnelSchlick(float cosTheta, vec3 F0)
        {
            return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
        }  

        float DistributionGGX(vec3 N, vec3 H, float roughness)
        {
            float a      = roughness*roughness;
            float a2     = a*a;
            float NdotH  = max(dot(N, H), 0.0);
            float NdotH2 = NdotH*NdotH;
	
            float num   = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;
	
            return num / denom;
        }

        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0);
            float k = (r*r) / 8.0;

            float num   = NdotV;
            float denom = NdotV * (1.0 - k) + k;
	
            return num / denom;
        }

        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            float ggx2  = GeometrySchlickGGX(NdotV, roughness);
            float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
            return ggx1 * ggx2;
        }

        void main()
        {
            vec3 N = normalize(vNormal);
            vec3 V = normalize(camPos - vWorldPos);

            // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
            // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
            vec3 F0 = vec3(0.04); 
            F0 = mix(F0, albedo, metallic);

            vec3 Lo = vec3(0.0);
            for(int i = 0; i < 4; ++i)
            {
                ///Per-light radiance
                vec3 L = normalize(lightPositions[i] - vWorldPos);
                vec3 H = normalize(V + L);

                float distance    = length(lightPositions[i] - vWorldPos);
                float attenuation = 1.0 / (distance * distance);
                vec3 radiance     = lightColors[i] * attenuation;
                ///
                    
                ///Cook-Torrance BRDF
                float NDF = DistributionGGX(N, H, roughness);   
                float G   = GeometrySmith(N, V, L, roughness);  
                vec3 F  = FresnelSchlick(max(dot(H, V), 0.0), F0);

                vec3 nominator    = NDF * G * F; 
                float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
                vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
                ///

                vec3 kS = F;
                vec3 kD = vec3(1.0) - kS;
                
                kD *= 1.0 - metallic;

                // scale light by NdotL
                float NdotL = max(dot(N, L), 0.0);

                // add to outgoing radiance Lo
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
            }
            
            vec3 ambiant = vec3(0.03) * albedo * ao;
            vec3 color = ambiant + Lo;
            
            //HDR
            color = color / (color + vec3(1.0));

            //Gamma correction
            color = pow(color,vec3(1.0/2.2));

            fragColor = vec4(color,1);
        }
        )GLSL"
    );

    texturedPBR.id = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec3 aNormal;            

        out vec2 vUV;
        out vec3 vWorldPos;
        out vec3 vNormal;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            vUV = aUV;
            vWorldPos = vec3(model * vec4(aPosition,1.0));
            vNormal = mat3(model) * aNormal;

            gl_Position = projection * view * vec4(vWorldPos, 1.0);
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        out vec4 fragColor;

        in vec2 vUV;
        in vec3 vWorldPos;
        in vec3 vNormal;

        uniform vec3 camPos;
        
        //Material
        uniform sampler2D albedoMap;
        uniform sampler2D normalMap;
        uniform sampler2D metallicMap;
        uniform sampler2D roughnessMap;
        uniform sampler2D aoMap;

        //Lights
        uniform vec3 lightPositions;
        uniform vec3 lightColors;

        const float PI = 3.14159265359;
        
        // ----------------------------------------------------------------------------
        // Easy trick to get tangent-normals to world-space to keep PBR code simplified.
        // Don't worry if you don't get what's going on; you generally want to do normal 
        // mapping the usual way for performance anways; I do plan make a note of this 
        // technique somewhere later in the normal mapping tutorial.
        vec3 getNormalFromMap()
        {
            vec3 tangentNormal = texture(normalMap, vUV).xyz * 2.0 - 1.0;

            vec3 Q1  = dFdx(vWorldPos);
            vec3 Q2  = dFdy(vWorldPos);
            vec2 st1 = dFdx(vUV);
            vec2 st2 = dFdy(vUV);

            vec3 N   = normalize(vNormal);
            vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
            vec3 B  = -normalize(cross(N, T));
            mat3 TBN = mat3(T, B, N);

            return normalize(TBN * tangentNormal);
        }
        // ----------------------------------------------------------------------------
        float DistributionGGX(vec3 N, vec3 H, float roughness)
        {
            float a = roughness*roughness;
            float a2 = a*a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH*NdotH;

            float nom   = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;

            return nom / denom;
        }
        // ----------------------------------------------------------------------------
        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0);
            float k = (r*r) / 8.0;

            float nom   = NdotV;
            float denom = NdotV * (1.0 - k) + k;

            return nom / denom;
        }
        // ----------------------------------------------------------------------------
        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            float ggx2 = GeometrySchlickGGX(NdotV, roughness);
            float ggx1 = GeometrySchlickGGX(NdotL, roughness);

            return ggx1 * ggx2;
        }
        // ----------------------------------------------------------------------------
        vec3 fresnelSchlick(float cosTheta, vec3 F0)
        {
            return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
        }

        void main()
        {
            vec3 albedo     = pow(texture(albedoMap, vUV).rgb, vec3(2.2));
            float metallic  = texture(metallicMap, vUV).r;
            float roughness = texture(roughnessMap, vUV).r;
            float ao        = texture(aoMap, vUV).r;

            vec3 N = getNormalFromMap();
            vec3 V = normalize(camPos - vWorldPos);

            // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
            // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
            vec3 F0 = vec3(0.04); 
            F0 = mix(F0, albedo, metallic);

            // reflectance equation
            vec3 Lo = vec3(0.0);

            // calculate per-light radiance
            vec3 L = normalize(lightPositions - vWorldPos);
            vec3 H = normalize(V + L);
            float distance = length(lightPositions - vWorldPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = lightColors * attenuation;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);   
            float G   = GeometrySmith(N, V, L, roughness);      
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
            vec3 nominator    = NDF * G * F; 
            float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
            vec3 specular = nominator / denominator;
        
            // kS is equal to Fresnel
            vec3 kS = F;
            // for energy conservation, the diffuse and specular light can't
            // be above 1.0 (unless the surface emits light); to preserve this
            // relationship the diffuse component (kD) should equal 1.0 - kS.
            vec3 kD = vec3(1.0) - kS;
            // multiply kD by the inverse metalness such that only non-metals 
            // have diffuse lighting, or a linear blend if partly metal (pure metals
            // have no diffuse light).
            kD *= 1.0 - metallic;	  

            // scale light by NdotL
            float NdotL = max(dot(N, L), 0.0);        

            // add to outgoing radiance Lo
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again            

            // ambient lighting (note that the next IBL tutorial will replace 
            // this ambient lighting with environment lighting).
            vec3 ambient = vec3(0.03) * albedo * ao;
    
            vec3 color = ambient + Lo;

            // HDR tonemapping
            color = color / (color + vec3(1.0));
            // gamma correct
            color = pow(color, vec3(1.0/2.2)); 

            fragColor = vec4(color, 1.0);
            //fragColor = vec4(texture(aoMap, vUV).rgb, 1.0);
        }
        )GLSL"
    );

    {
        glGenTextures(1, &pbrSphere.albedo);
        glBindTexture(GL_TEXTURE_2D, pbrSphere.albedo);
        gl::UploadImage("media/Mat_Albedo.jpg");
        gl::SetTextureDefaultParams();

        glGenTextures(1, &pbrSphere.normal);
        glBindTexture(GL_TEXTURE_2D, pbrSphere.normal);
        gl::UploadImage("media/Mat_Normal.jpg");
        gl::SetTextureDefaultParams();

        glGenTextures(1, &pbrSphere.metallic);
        glBindTexture(GL_TEXTURE_2D, pbrSphere.metallic);
        gl::UploadImage("media/Mat_Metallic.jpg");
        gl::SetTextureDefaultParams();

        glGenTextures(1, &pbrSphere.roughness);
        glBindTexture(GL_TEXTURE_2D, pbrSphere.roughness);
        gl::UploadImage("media/Mat_Roughness.jpg");
        gl::SetTextureDefaultParams();

        glGenTextures(1, &pbrSphere.ao);
        glBindTexture(GL_TEXTURE_2D, pbrSphere.ao);
        gl::UploadImage("media/Mat_AO.jpg");
        gl::SetTextureDefaultParams();
    }
}

DemoPBR::~DemoPBR()
{
    glDeleteProgram(basicPBR.id);
    glDeleteProgram(texturedPBR.id);
    glDeleteVertexArrays(1, &pbrSphere.VAO);
    glDeleteBuffers(1, &pbrSphere.VBO);
    glDeleteTextures(1, &pbrSphere.albedo);
    glDeleteTextures(1, &pbrSphere.normal);
    glDeleteTextures(1, &pbrSphere.metallic);
    glDeleteTextures(1, &pbrSphere.roughness);
    glDeleteTextures(1, &pbrSphere.ao);
}

void DemoPBR::UpdateAndRender(const DemoInputs& inputs)
{
    glEnable(GL_DEPTH_TEST);
    mainCamera.UpdateFreeFly(inputs.cameraInputs);
    glClearColor(0.33, 0.33, 0.33, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);

    static bool usePBRTexture = false;

    {
        ImGui::DragFloat3("Light pos", lights.position.e);
        ImGui::ColorEdit3("Light color", lights.color.e);

        static int e = 0;
        ImGui::RadioButton("Basic PBR", &e, 0);
        ImGui::RadioButton("Textured PBR", &e, 1);

        if (e == 0)
        {
            usePBRTexture = false;
            usedProgram = basicPBR;
        }
        else
        {
            usePBRTexture = true;
            usedProgram = texturedPBR;

            ImGui::Text("Albedo");
            ImGui::Image((ImTextureID)(size_t)pbrSphere.albedo, { 256, 256 });

            ImGui::Text("Normal");
            ImGui::Image((ImTextureID)(size_t)pbrSphere.normal, { 256, 256 });

            ImGui::Text("Metallic");
            ImGui::Image((ImTextureID)(size_t)pbrSphere.metallic, { 256, 256 });

            ImGui::Text("Roughness");
            ImGui::Image((ImTextureID)(size_t)pbrSphere.roughness, { 256, 256 });

            ImGui::Text("Ambiant Occlusion");
            ImGui::Image((ImTextureID)(size_t)pbrSphere.ao, { 256, 256 });
        }
    }

    {
        mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
        mat4 view = mainCamera.GetViewMatrix();
        mat4 model = mat4Scale(1.f);

        glUseProgram(usedProgram.id);
        glUniformMatrix4fv(glGetUniformLocation(usedProgram.id, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(usedProgram.id, "view"), 1, GL_FALSE, view.e);

        glUniform3f(glGetUniformLocation(usedProgram.id,"camPos"), mainCamera.position.x, mainCamera.position.y, mainCamera.position.z);

        if (usePBRTexture)
        {
            glUniform1i(glGetUniformLocation(usedProgram.id, "albedoMap"), 0);
            glUniform1i(glGetUniformLocation(usedProgram.id, "normalMap"), 1);
            glUniform1i(glGetUniformLocation(usedProgram.id, "metallicMap"), 2);
            glUniform1i(glGetUniformLocation(usedProgram.id, "roughnessMap"), 3);
            glUniform1i(glGetUniformLocation(usedProgram.id, "aoMap"), 4);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pbrSphere.albedo);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pbrSphere.normal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, pbrSphere.metallic);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, pbrSphere.roughness);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, pbrSphere.ao);
        }
        else
        {
            glUniform3f(glGetUniformLocation(usedProgram.id, "albedo"), 0.5f, 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(usedProgram.id, "ao"), 1.0f);
        }

        for (int row = 0; row < nrRows; ++row)
        {
            if(!usePBRTexture)
                glUniform1f(glGetUniformLocation(usedProgram.id, "metallic"), (float)row / (float)nrRows);
            for (int col = 0; col < nrColumns; ++col)
            {
                // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
                // on direct lighting.
                if (!usePBRTexture)
                    glUniform1f(glGetUniformLocation(usedProgram.id, "roughness"), calc::Clamp<float>((float)col / (float)nrColumns, 0.05f, 1.0f));

                model = mat4Identity();
                model = mat4Translate(model, float3(
                    (col - (nrColumns / 2)) * spacing,
                    (row - (nrRows / 2)) * spacing,
                    0.0f
                ));
                glUniformMatrix4fv(glGetUniformLocation(usedProgram.id, "model"), 1, GL_FALSE, model.e);

                glBindVertexArray(pbrSphere.VAO);
                glDrawArrays(GL_TRIANGLES, pbrSphere.mesh.start, pbrSphere.mesh.count);
            }
        }
        
        float3 newPos = lights.position + float3(sinf(glfwGetTime() * 5.0f) * 5.0f, 0.0f, 0.0f);
        newPos = lights.position;
        glUniform3f(glGetUniformLocation(usedProgram.id, "lightPositions"), newPos.x, newPos.y, newPos.z);
        glUniform3f(glGetUniformLocation(usedProgram.id, "lightColors"), lights.color.x, lights.color.y, lights.color.z);

        model = mat4Identity();
        model = mat4Translate(model, newPos);
        model = mat4Scale(model, 0.5f);
        glUniformMatrix4fv(glGetUniformLocation(usedProgram.id, "model"), 1, GL_FALSE, model.e);

        glBindVertexArray(pbrSphere.VAO);
        glDrawArrays(GL_TRIANGLES, pbrSphere.mesh.start, pbrSphere.mesh.count);
    }
}