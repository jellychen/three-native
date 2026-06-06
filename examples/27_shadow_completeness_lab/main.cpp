#include "platform/Window.hpp"
#include "renderer/GLRenderer.hpp"
#include "core/Renderable.hpp"
#include "light/Light.hpp"
#include "helpers/GeometryFactory.hpp"
#include "helpers/Helpers.hpp"
#include "texture/TextureFactory.hpp"
#include "ibl/Environment.hpp"
#include "controls/OrbitControls.hpp"

using namespace threecpp;

int main() {
    Window window(1440, 860, "threecpp v3.5 ShadowMap completeness lab - Directional + Spot + moving casters");
    Scene scene;
    scene.backgroundColor = {0.018f,0.020f,0.028f};
    PMREMGenerator pmrem({64, 6, true, 32, 32});
    scene.environment = pmrem.fromEquirectangular(TextureFactory::makeStudioHDRI(256, 128));
    scene.environmentIntensity = 0.75f;

    PerspectiveCamera camera(50.0f, 1440.0f/860.0f, 0.05f, 200.0f);
    camera.position = {8,5.5f,8};
    camera.lookAt({0,0.8f,0});
    OrbitControls controls(camera, window);
    controls.target = {0,0.8f,0};

    auto amb = make_ref<AmbientLight>(); amb->intensity = 0.25f; scene.add(amb);
    auto sun = make_ref<DirectionalLight>();
    sun->position = {6,10,7}; sun->target = {0,0,0}; sun->intensity = 4.5f; sun->castShadow = true; sun->shadow.enabled = true;
    sun->shadow.mapSizeX = 2048; sun->shadow.mapSizeY = 2048; sun->shadow.cameraLeft=-12; sun->shadow.cameraRight=12; sun->shadow.cameraBottom=-12; sun->shadow.cameraTop=12; sun->shadow.cameraFar=60; sun->shadow.bias=0.001; sun->shadow.radius=1.6f; scene.add(sun);
    auto spot = make_ref<SpotLight>();
    spot->position = {-5,6,4}; spot->target = {0,0.5f,0}; spot->intensity = 75; spot->distance=18; spot->angle=glm::radians(25.0f); spot->penumbra=0.55f; spot->castShadow=true; spot->shadow.enabled=true; spot->shadow.mapSizeX=1024; spot->shadow.mapSizeY=1024; spot->shadow.bias=0.0008f; spot->shadow.radius=1.2f; scene.add(spot);
    auto point = make_ref<PointLight>(); point->position={3,2.5f,-4}; point->color={1,0.45f,0.2f}; point->intensity=55; point->distance=15; scene.add(point);

    auto groundMat = make_ref<MeshStandardMaterial>(); groundMat->color={0.48f,0.48f,0.45f}; groundMat->roughness=0.86f; groundMat->map=TextureFactory::makeCheckerboard(512,512,16,{0.58f,0.58f,0.54f},{0.32f,0.33f,0.34f}); groundMat->map->repeat={6,6};
    auto ground = make_ref<Mesh>(GeometryFactory::makePlaneSegments(22, 16, 16, 12), groundMat); ground->receiveShadow=true; scene.add(ground);

    auto sphere = GeometryFactory::makeUVSphere(0.55f,48,24); auto cube = GeometryFactory::makeCube(1.0f); auto torus=GeometryFactory::makeTorus(0.45f,0.15f,48,14);
    std::vector<std::shared_ptr<Mesh>> movers;
    for(int i=0;i<12;++i){ auto mat=make_ref<MeshStandardMaterial>(); mat->color=glm::mix(glm::vec3(1,0.38f,0.18f), glm::vec3(0.28f,0.62f,1), float(i)/11.0f); mat->roughness=0.22f+0.55f*float(i%4)/3.0f; mat->metalness=(i%5==0)?0.65f:0.0f; auto geo=(i%3==0)?sphere:((i%3==1)?cube:torus); auto mesh=make_ref<Mesh>(geo,mat); mesh->position={float(i%6-2.5f)*1.45f,0.65f,float(i/6)*2.0f-1.0f}; mesh->castShadow=true; mesh->receiveShadow=true; scene.add(mesh); if(i%3==0)movers.push_back(mesh); }
    auto gridMat=make_ref<LineBasicMaterial>(); gridMat->color={0.10f,0.11f,0.13f}; auto grid=make_ref<LineSegments>(GeometryFactory::makeGrid(22,22),gridMat); grid->position.y=0.012f; scene.add(grid);

    GLRenderer renderer({1440,860}); renderer.setToneMapping(ToneMapping::ACESFilmic,1.0f);
    while(!window.shouldClose()){
        window.poll(); controls.update(); float t=float(window.time());
        sun->position={std::sin(t*0.25f)*7.0f,10.0f,std::cos(t*0.25f)*7.0f};
        spot->position={std::sin(t*0.7f)*5.0f,6.0f,std::cos(t*0.7f)*4.0f}; spot->target={std::sin(t*1.3f)*1.2f,0.5f,std::cos(t*1.1f)*1.2f};
        point->position={std::cos(t)*4.0f,2.5f,std::sin(t)*4.0f};
        for(size_t i=0;i<movers.size();++i){ movers[i]->position.y=0.65f+std::abs(std::sin(t*1.2f+float(i)))*1.0f; movers[i]->rotation.y=t*0.7f+float(i); }
        renderer.render(scene,camera); window.swapBuffers();
    }
}
