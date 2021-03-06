#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

#include "Application/Timing.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad | AppLayerFunctions::OnUpdate;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}



void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		//MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("Cube.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		Texture2D::Sptr    woodTex      = ResourceManager::CreateAsset<Texture2D>("textures/wood.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);


		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");  

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.Diffuse", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr platMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			platMaterial->Name = "Wood";
			platMaterial->Set("u_Material.Diffuse", woodTex);
			platMaterial->Set("u_Material.Shininess", 0.1f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(reflectiveShader);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.Diffuse", monkeyTex);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.Diffuse", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
		}

		// because a dj should be shmooving
		Material::Sptr djMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			djMaterial->Name = "dj Shader";
			djMaterial->Set("u_Material.Diffuse", boxTexture);
			djMaterial->Set("u_Material.Specular", boxSpec);
			djMaterial->Set("u_Material.Shininess", 0.1f);
			djMaterial->Set("u_Material.Threshold", 0.1f);

			djMaterial->Set("u_WindDirection", glm::vec3(0.0f, 1.0f, 0.5f));
			djMaterial->Set("u_WindStrength", 0.5f);
			djMaterial->Set("u_VerticalScale", 1.0f);
			djMaterial->Set("u_WindSpeed", 9.0f);
		}

		// Our foliage vertex shader material
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.Diffuse", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.Threshold", 0.1f);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.Diffuse", boxTexture);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.Diffuse", diffuseMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("s_NormalMap", normalMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(tangentSpaceMapping);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.Diffuse", diffuseMap);
			normalmapMat->Set("s_NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}

		// Create some lights for our scene
		scene->Lights.resize(5);
		scene->Lights[0].Position = glm::vec3(0.0f, -6.5f, 24.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		
		scene->Lights[1].Position = glm::vec3(-3.677f, -0.269f, 3.0f);
		scene->Lights[1].Color = glm::vec3(6.0f, 6.0f, 0.0f);
		scene->Lights[1].Range = 2.0f;

		scene->Lights[2].Position = glm::vec3(3.077f, -1.836f, 3.0f);
		scene->Lights[2].Color = glm::vec3(0.0f, 0.0f, 8.0f);
		scene->Lights[2].Range = 2.0f;

		scene->Lights[3].Position = glm::vec3(-4.012f, -8.852f, 3.0f);
		scene->Lights[3].Color = glm::vec3(8.0f, 0.0f, 0.0f);
		scene->Lights[3].Range = 2.0f;

		scene->Lights[4].Position = glm::vec3(2.987f, -9.268f, 3.0f);
		scene->Lights[4].Color = glm::vec3(0.0f, 8.0f, 0.0f);
		scene->Lights[4].Range = 2.0f;
		

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPosition({ -9, -6, 15 });
			//camera->LookAt(glm::vec3(0.0f));
			camera->SetRotation({ 90, 0, 90 });

			//camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			plane->SetPosition(glm::vec3(0.170f, -6.5f, 0.0f));
			plane->SetScale(glm::vec3(1.0f, 20.f, 1.0f));

			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}
		/*
		GameObject::Sptr monkey1 = scene->CreateGameObject("Monkey 1");
		{
			// Set position in the scene
			monkey1->SetPosition(glm::vec3(1.5f, 0.0f, 1.0f));

			// Add some behaviour that relies on the physics body
			monkey1->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = monkey1->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(monkeyMaterial);

			// Example of a trigger that interacts with static and kinematic bodies as well as dynamic bodies
			TriggerVolume::Sptr trigger = monkey1->Add<TriggerVolume>();
			trigger->SetFlags(TriggerTypeFlags::Statics | TriggerTypeFlags::Kinematics);
			trigger->AddCollider(BoxCollider::Create(glm::vec3(1.0f)));

			monkey1->Add<TriggerVolumeEnterBehaviour>();
		}
		*/
		GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");
		demoBase->SetPosition(glm::vec3(20, 0, 0));

		// Box to showcase the specular material
		GameObject::Sptr specBox = scene->CreateGameObject("Specular Object");
		{
			MeshResource::Sptr boxMesh = ResourceManager::CreateAsset<MeshResource>();
			boxMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			boxMesh->GenerateMesh();

			// Set and rotation position in the scene
			specBox->SetPosition(glm::vec3(0, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = specBox->Add<RenderComponent>();
			renderer->SetMesh(boxMesh);
			renderer->SetMaterial(testMaterial);

			demoBase->AddChild(specBox);
		}

		// sphere to showcase the foliage material
		GameObject::Sptr foliageBall = scene->CreateGameObject("Foliage Sphere");
		{
			// Set and rotation position in the scene
			foliageBall->SetPosition(glm::vec3(-4.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = foliageBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(foliageMaterial);

			demoBase->AddChild(foliageBall);
		}

		// Box to showcase the foliage material
		GameObject::Sptr foliageBox = scene->CreateGameObject("Foliage Box");
		{
			MeshResource::Sptr box = ResourceManager::CreateAsset<MeshResource>();
			box->AddParam(MeshBuilderParam::CreateCube(glm::vec3(0, 0, 0.5f), ONE));
			box->GenerateMesh();

			// Set and rotation position in the scene
			foliageBox->SetPosition(glm::vec3(-6.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = foliageBox->Add<RenderComponent>();
			renderer->SetMesh(box);
			renderer->SetMaterial(foliageMaterial);

			demoBase->AddChild(foliageBox);
		}

		// Box to showcase the specular material
		GameObject::Sptr toonBall = scene->CreateGameObject("Toon Object");
		{
			// Set and rotation position in the scene
			toonBall->SetPosition(glm::vec3(-2.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = toonBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(toonMaterial);

			demoBase->AddChild(toonBall);
		}

		GameObject::Sptr displacementBall = scene->CreateGameObject("Displacement Object");
		{
			// Set and rotation position in the scene
			displacementBall->SetPosition(glm::vec3(2.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = displacementBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(displacementTest);

			demoBase->AddChild(displacementBall);
		}

		GameObject::Sptr multiTextureBall = scene->CreateGameObject("Multitextured Object");
		{
			// Set and rotation position in the scene 
			multiTextureBall->SetPosition(glm::vec3(4.0f, -4.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(multiTextureMat);

			demoBase->AddChild(multiTextureBall);
		}

		GameObject::Sptr normalMapBall = scene->CreateGameObject("Normal Mapped Object");
		{
			// Set and rotation position in the scene 
			normalMapBall->SetPosition(glm::vec3(6.0f, -4.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = normalMapBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(normalmapMat);

			demoBase->AddChild(normalMapBall);
		}

		// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr trigger = scene->CreateGameObject("Trigger");
		{
			TriggerVolume::Sptr volume = trigger->Add<TriggerVolume>();
			CylinderCollider::Sptr collider = CylinderCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
			collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
			volume->AddCollider(collider);

			trigger->Add<TriggerVolumeEnterBehaviour>();
		}

		/////////////////////////// UI //////////////////////////////
		/*
		GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);

				monkey1->Get<JumpBehaviour>()->Panel = text;
			}

			canvas->AddChild(subPanel);
		}
		*/

		/////////////////////////////////////////////////////////////								New shtuff								 /////////////////////////////////////////////////////////


		GameObject::Sptr multiTexturefloor = scene->CreateGameObject("Multitextured floor");
		{
			MeshResource::Sptr multiTexturefloorMesh = ResourceManager::CreateAsset<MeshResource>();
			multiTexturefloorMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			multiTexturefloorMesh->GenerateMesh();

			// Set and rotation position in the scene 
			multiTexturefloor->SetPosition(glm::vec3(0.0f, -27.5f, -0.450f));
			multiTexturefloor->SetScale(glm::vec3(25.0f, 25.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTexturefloor->Add<RenderComponent>();
			renderer->SetMesh(multiTexturefloorMesh);
			renderer->SetMaterial(multiTextureMat);

		}

		// make walls 1 - 3
		MeshResource::Sptr displacementWallMesh = ResourceManager::CreateAsset<MeshResource>();
		displacementWallMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
		displacementWallMesh->GenerateMesh();

		GameObject::Sptr displacementWall = scene->CreateGameObject("Displacement Wall");
		{
			// Set and rotation position in the scene
			displacementWall->SetPosition(glm::vec3(0.0f, 5.0f, 9.0f));
			displacementWall->SetScale(glm::vec3(20.0f, 1.0f, 20.0f));

			// Add a render component
			RenderComponent::Sptr renderer = displacementWall->Add<RenderComponent>();
			renderer->SetMesh(displacementWallMesh);
			renderer->SetMaterial(displacementTest);
			
			RigidBody::Sptr physics = displacementWall->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());
		}

		GameObject::Sptr displacementWall2 = scene->CreateGameObject("Displacement Wall2");
		{
			// Set and rotation position in the scene
			displacementWall2->SetPosition(glm::vec3(10.0f, -5.0f, 5.0f));
			displacementWall2->SetScale(glm::vec3(20.0f, 1.0f, 20.0f));
			displacementWall2->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Add a render component
			RenderComponent::Sptr renderer = displacementWall2->Add<RenderComponent>();
			renderer->SetMesh(displacementWallMesh);
			renderer->SetMaterial(displacementTest);

			RigidBody::Sptr physics = displacementWall2->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());
		}

		GameObject::Sptr displacementWall3 = scene->CreateGameObject("Displacement Wall3");
		{
			// Set and rotation position in the scene
			displacementWall3->SetPosition(glm::vec3(-10.0f, -5.0f, -1.0f));
			displacementWall3->SetScale(glm::vec3(20.0f, 1.0f, 20.0f));
			displacementWall3->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Add a render component
			RenderComponent::Sptr renderer = displacementWall3->Add<RenderComponent>();
			renderer->SetMesh(displacementWallMesh);
			renderer->SetMaterial(displacementTest);

			RigidBody::Sptr physics = displacementWall3->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());
		}


		GameObject::Sptr player = scene->CreateGameObject("player");
		{
			

			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			player->SetPosition(glm::vec3(0.f, -10.f, 1.f));
			player->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = player->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = player->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			/*
			Light::Sptr playerLight = player->Add<Light>();
			playerLight->SetColor({ 1.0f, 1.0f, 1.0f });
			playerLight->SetRadius(-4.5f);
			playerLight->SetIntensity(1.0f);
			*/
		}

		GameObject::Sptr crowdBase = scene->CreateGameObject("Crowd");

		GameObject::Sptr person1 = scene->CreateGameObject("person1");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person1->SetPosition(glm::vec3(-4.68f, -3.980f, 1.f));
			person1->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person1->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person1->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person1);
		}

		GameObject::Sptr person2 = scene->CreateGameObject("person2");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person2->SetPosition(glm::vec3(2.240f, -7.360f, 1.f));
			person2->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person2->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person2->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person2);
		}

		GameObject::Sptr person3 = scene->CreateGameObject("person3");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person3->SetPosition(glm::vec3(4.33f, -5.14f, 1.f));
			person3->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person3->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person3->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person3);
		}

		GameObject::Sptr person4 = scene->CreateGameObject("person4");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person4->SetPosition(glm::vec3(-2.350f, -9.12f, 1.f));
			person4->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person4->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person4->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person4);
		}

		GameObject::Sptr person5 = scene->CreateGameObject("person5");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person5->SetPosition(glm::vec3(4.66f, -9.02f, 1.f));
			person5->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person5->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person5->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person5);
		}

		GameObject::Sptr person6 = scene->CreateGameObject("person6");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person6->SetPosition(glm::vec3(-4.f, -11.880f, 1.f));
			person6->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person6->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person6->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person6);
		}

		GameObject::Sptr person7 = scene->CreateGameObject("person7");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person7->SetPosition(glm::vec3(1.825f, -3.028f, 1.f));
			person7->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person7->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person7->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person7);
		}

		GameObject::Sptr person8 = scene->CreateGameObject("person8");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			person8->SetPosition(glm::vec3(-5.755f, -7.937f, 1.f));
			person8->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = person8->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(testMaterial);

			RigidBody::Sptr physics = person8->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

			crowdBase->AddChild(person8);
		}

		GameObject::Sptr dj = scene->CreateGameObject("dj");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			dj->SetPosition(glm::vec3(0.f, 1.139f, 2.3f));
			dj->SetScale(glm::vec3(1.5f));

			RenderComponent::Sptr renderer = dj->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(djMaterial);

			RigidBody::Sptr physics = dj->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());
		}

		GameObject::Sptr platform = scene->CreateGameObject("platform");
		{
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cubeMesh->GenerateMesh();

			platform->SetPosition(glm::vec3(0.0f, 1.5f, 1.f));
			platform->SetScale(glm::vec3(10.f, 5.f, 1.f));

			RenderComponent::Sptr renderer = platform->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(platMaterial);

			RigidBody::Sptr physics = platform->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create());

		}



		GameObject::Sptr particles = scene->CreateGameObject("Particles");
		{
			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();  
			particleManager->AddEmitter(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 10.0f), 10.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); 
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);

		
	}
}


void DefaultSceneLayer::OnUpdate() {
	//Update game loop
	Application& app = Application::Get(); //get application
	_currentScene = app.CurrentScene(); //get The Scene


	//time things
	float dt = Timing::Current().DeltaTime();

	if (glfwGetKey(app.GetWindow(), GLFW_KEY_A) == GLFW_PRESS) {
		moveLeft = true;
	}

	else if (glfwGetKey(app.GetWindow(), GLFW_KEY_A) == GLFW_RELEASE) {
		moveLeft = false;
	}

	if (glfwGetKey(app.GetWindow(), GLFW_KEY_D) == GLFW_PRESS) {
		moveRight = true;
	}

	else if (glfwGetKey(app.GetWindow(), GLFW_KEY_D) == GLFW_RELEASE) {
		moveRight = false;
	}

	if (glfwGetKey(app.GetWindow(), GLFW_KEY_S) == GLFW_PRESS) {
		moveDown = true;
	}

	else if (glfwGetKey(app.GetWindow(), GLFW_KEY_S) == GLFW_RELEASE) {
		moveDown = false;
	}

	if (glfwGetKey(app.GetWindow(), GLFW_KEY_W) == GLFW_PRESS) {
		moveUp = true;
	}

	else if (glfwGetKey(app.GetWindow(), GLFW_KEY_W) == GLFW_RELEASE) {
		moveUp = false;
	}

	if (InputEngine::GetKeyState(GLFW_KEY_F) == ButtonState::Pressed) {
		cameraTest = !cameraTest;
		std::cout << cameraTest;
	}

	if (moveLeft == true)
	{
		_currentScene->FindObjectByName("player")->SetPosition(glm::vec3(_currentScene->FindObjectByName("player")->GetPosition().x - (dt * 5), _currentScene->FindObjectByName("player")->GetPosition().y, _currentScene->FindObjectByName("player")->GetPosition().z));
	}

	if (moveRight == true)
	{
		_currentScene->FindObjectByName("player")->SetPosition(glm::vec3(_currentScene->FindObjectByName("player")->GetPosition().x + (dt * 5), _currentScene->FindObjectByName("player")->GetPosition().y, _currentScene->FindObjectByName("player")->GetPosition().z));
	}

	if (moveUp == true)
	{
		_currentScene->FindObjectByName("player")->SetPosition(glm::vec3(_currentScene->FindObjectByName("player")->GetPosition().x, _currentScene->FindObjectByName("player")->GetPosition().y + (dt * 5), _currentScene->FindObjectByName("player")->GetPosition().z));
	}

	if (moveDown == true)
	{
		_currentScene->FindObjectByName("player")->SetPosition(glm::vec3(_currentScene->FindObjectByName("player")->GetPosition().x, _currentScene->FindObjectByName("player")->GetPosition().y - (dt * 5), _currentScene->FindObjectByName("player")->GetPosition().z));
	}

	if (cameraTest == true)
	{
		_currentScene->MainCamera->GetGameObject()->SetPosition(glm::vec3(_currentScene->FindObjectByName("player")->GetPosition().x, _currentScene->FindObjectByName("player")->GetPosition().y - 3, _currentScene->FindObjectByName("player")->GetPosition().z + 4.5));
		_currentScene->MainCamera->GetGameObject()->LookAt(_currentScene->FindObjectByName("player")->GetPosition());

	}
	else {
		_currentScene->MainCamera->GetGameObject()->SetPosition(glm::vec3(-9, -6, 15));
		_currentScene->MainCamera->GetGameObject()->LookAt({ 0, 0, 0 });
	}

	if (currTime > 0) {
		currTime -= dt;
	}

	if (currTime <= 0)
	{
		currTime = glm::linearRand(0.5f, 2.0f);

		colourpick = glm::linearRand(1, 4);
	}

	//if time > 0 go down
	//if time 0, jump for period of time
	//after period of time make new time before jump again
	
	//le super scuffed jumps

	//box1jump
	if (jump1 > 0 && jumping1 == false && jumped1 == false)
		{
			jump1 -= dt;
		}

	if (jump1 <= 0 && jumping1 == false)
	{
		jumping1 = true;
		jump1 = 0.2;
	}

	if (jumping1 == true && _currentScene->FindObjectByName("person1")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person1")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person1")->GetPosition().x, _currentScene->FindObjectByName("person1")->GetPosition().y, _currentScene->FindObjectByName("person1")->GetPosition().z + (dt * 4)));

	}
	else if (jumping1 == true && _currentScene->FindObjectByName("person1")->GetPosition().z >= 1.5)
	{
		jumped1 = true;
		jumping1 = false;
	}

	if (jumped1 == true && _currentScene->FindObjectByName("person1")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person1")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person1")->GetPosition().x, _currentScene->FindObjectByName("person1")->GetPosition().y, _currentScene->FindObjectByName("person1")->GetPosition().z - (dt * 4)));
	}
	else if (jumped1 == true && _currentScene->FindObjectByName("person1")->GetPosition().z <= 1)
	{
		jumped1 = false;
		jump1 = glm::linearRand(0.1f, 0.5f);
	}

	// box2jump
	if (jump2 > 0 && jumping2 == false && jumped2 == false)
	{
		jump2 -= dt;
	}

	if (jump2 <= 0 && jumping2 == false)
	{
		jumping2 = true;
		jump2 = 0.2;
	}

	if (jumping2 == true && _currentScene->FindObjectByName("person2")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person2")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person2")->GetPosition().x, _currentScene->FindObjectByName("person2")->GetPosition().y, _currentScene->FindObjectByName("person2")->GetPosition().z + (dt * 4)));

	}
	else if (jumping2 == true && _currentScene->FindObjectByName("person2")->GetPosition().z >= 1.5)
	{
		jumped2 = true;
		jumping2 = false;
	}

	if (jumped2 == true && _currentScene->FindObjectByName("person2")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person2")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person2")->GetPosition().x, _currentScene->FindObjectByName("person2")->GetPosition().y, _currentScene->FindObjectByName("person2")->GetPosition().z - (dt * 4)));
	}
	else if (jumped2 == true && _currentScene->FindObjectByName("person2")->GetPosition().z <= 1)
	{
		jumped2 = false;
		jump2 = glm::linearRand(0.1f, 0.5f);
	}

	//box3jump
	if (jump3 > 0 && jumping3 == false && jumped3 == false)
	{
		jump3 -= dt;
	}

	if (jump3 <= 0 && jumping3 == false)
	{
		jumping3 = true;
		jump3 = 0.2;
	}

	if (jumping3 == true && _currentScene->FindObjectByName("person3")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person3")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person3")->GetPosition().x, _currentScene->FindObjectByName("person3")->GetPosition().y, _currentScene->FindObjectByName("person3")->GetPosition().z + (dt * 4)));

	}
	else if (jumping3 == true && _currentScene->FindObjectByName("person3")->GetPosition().z >= 1.5)
	{
		jumped3 = true;
		jumping3 = false;
	}

	if (jumped3 == true && _currentScene->FindObjectByName("person3")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person3")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person3")->GetPosition().x, _currentScene->FindObjectByName("person3")->GetPosition().y, _currentScene->FindObjectByName("person3")->GetPosition().z - (dt * 4)));
	}
	else if (jumped3 == true && _currentScene->FindObjectByName("person3")->GetPosition().z <= 1)
	{
		jumped3 = false;
		jump3 = glm::linearRand(0.1f, 0.5f);
	}

	//box4jump

	if (jump4 > 0 && jumping4 == false && jumped4 == false)
	{
		jump4 -= dt;
	}

	if (jump4 <= 0 && jumping4 == false)
	{
		jumping4 = true;
		jump4 = 0.2;
	}

	if (jumping4 == true && _currentScene->FindObjectByName("person4")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person4")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person4")->GetPosition().x, _currentScene->FindObjectByName("person4")->GetPosition().y, _currentScene->FindObjectByName("person4")->GetPosition().z + (dt * 4)));

	}
	else if (jumping4 == true && _currentScene->FindObjectByName("person4")->GetPosition().z >= 1.5)
	{
		jumped4 = true;
		jumping4 = false;
	}

	if (jumped4 == true && _currentScene->FindObjectByName("person4")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person4")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person4")->GetPosition().x, _currentScene->FindObjectByName("person4")->GetPosition().y, _currentScene->FindObjectByName("person4")->GetPosition().z - (dt * 4)));
	}
	else if (jumped4 == true && _currentScene->FindObjectByName("person4")->GetPosition().z <= 1)
	{
		jumped4 = false;
		jump4 = glm::linearRand(0.1f, 0.5f);
	}

	//box5jump
	if (jump5 > 0 && jumping5 == false && jumped5 == false)
	{
		jump5 -= dt;
	}

	if (jump5 <= 0 && jumping5 == false)
	{
		jumping5 = true;
		jump5 = 0.2;
	}

	if (jumping5 == true && _currentScene->FindObjectByName("person5")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person5")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person5")->GetPosition().x, _currentScene->FindObjectByName("person5")->GetPosition().y, _currentScene->FindObjectByName("person5")->GetPosition().z + (dt * 4)));

	}
	else if (jumping5 == true && _currentScene->FindObjectByName("person5")->GetPosition().z >= 1.5)
	{
		jumped5 = true;
		jumping5 = false;
	}

	if (jumped5 == true && _currentScene->FindObjectByName("person5")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person5")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person5")->GetPosition().x, _currentScene->FindObjectByName("person5")->GetPosition().y, _currentScene->FindObjectByName("person5")->GetPosition().z - (dt * 4)));
	}
	else if (jumped5 == true && _currentScene->FindObjectByName("person5")->GetPosition().z <= 1)
	{
		jumped5 = false;
		jump5 = glm::linearRand(0.1f, 0.5f);
	}

	//box6jump
	if (jump6 > 0 && jumping6 == false && jumped6 == false)
	{
		jump6 -= dt;
	}

	if (jump6 <= 0 && jumping6 == false)
	{
		jumping6 = true;
		jump6 = 0.2;
	}

	if (jumping6 == true && _currentScene->FindObjectByName("person6")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person6")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person6")->GetPosition().x, _currentScene->FindObjectByName("person6")->GetPosition().y, _currentScene->FindObjectByName("person6")->GetPosition().z + (dt * 4)));

	}
	else if (jumping6 == true && _currentScene->FindObjectByName("person1")->GetPosition().z >= 1.5)
	{
		jumped6 = true;
		jumping6 = false;
	}

	if (jumped6 == true && _currentScene->FindObjectByName("person6")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person6")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person6")->GetPosition().x, _currentScene->FindObjectByName("person6")->GetPosition().y, _currentScene->FindObjectByName("person6")->GetPosition().z - (dt * 4)));
	}
	else if (jumped6 == true && _currentScene->FindObjectByName("person6")->GetPosition().z <= 1)
	{
		jumped6 = false;
		jump6 = glm::linearRand(0.1f, 0.5f);
	}

	//box7jump
	if (jump7 > 0 && jumping7 == false && jumped7 == false)
	{
		jump7 -= dt;
	}

	if (jump7 <= 0 && jumping7 == false)
	{
		jumping7 = true;
		jump7 = 0.2;
	}

	if (jumping7 == true && _currentScene->FindObjectByName("person7")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person7")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person7")->GetPosition().x, _currentScene->FindObjectByName("person7")->GetPosition().y, _currentScene->FindObjectByName("person7")->GetPosition().z + (dt * 4)));

	}
	else if (jumping7 == true && _currentScene->FindObjectByName("person7")->GetPosition().z >= 1.5)
	{
		jumped7 = true;
		jumping7 = false;
	}

	if (jumped7 == true && _currentScene->FindObjectByName("person7")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person7")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person7")->GetPosition().x, _currentScene->FindObjectByName("person7")->GetPosition().y, _currentScene->FindObjectByName("person7")->GetPosition().z - (dt * 4)));
	}
	else if (jumped7 == true && _currentScene->FindObjectByName("person7")->GetPosition().z <= 1)
	{
		jumped7 = false;
		jump7 = glm::linearRand(0.1f, 0.5f);
	}

	//box8jump
	if (jump8 > 0 && jumping8 == false && jumped8 == false)
	{
		jump8 -= dt;
	}

	if (jump8 <= 0 && jumping8 == false)
	{
		jumping8 = true;
		jump8 = 0.2;
	}

	if (jumping8 == true && _currentScene->FindObjectByName("person8")->GetPosition().z < 1.5)
	{
		_currentScene->FindObjectByName("person8")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person8")->GetPosition().x, _currentScene->FindObjectByName("person8")->GetPosition().y, _currentScene->FindObjectByName("person8")->GetPosition().z + (dt * 4)));

	}
	else if (jumping8 == true && _currentScene->FindObjectByName("person8")->GetPosition().z >= 1.5)
	{
		jumped8 = true;
		jumping8 = false;
	}

	if (jumped8 == true && _currentScene->FindObjectByName("person8")->GetPosition().z > 1)
	{
		_currentScene->FindObjectByName("person8")->SetPosition(glm::vec3(_currentScene->FindObjectByName("person8")->GetPosition().x, _currentScene->FindObjectByName("person8")->GetPosition().y, _currentScene->FindObjectByName("person8")->GetPosition().z - (dt * 4)));
	}
	else if (jumped8 == true && _currentScene->FindObjectByName("person8")->GetPosition().z <= 1)
	{
		jumped8 = false;
		jump8 = glm::linearRand(0.1f, 0.5f);
	}

	//Toggleables

	

	//No Lighting
	if (InputEngine::GetKeyState(GLFW_KEY_1) == ButtonState::Pressed) {
		app.CurrentScene()->SetAmbientLight(glm::vec3(0.0f));
		app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
		Luted = false;

	}

	//Ambient lighting only
	if (InputEngine::GetKeyState(GLFW_KEY_2) == ButtonState::Pressed) {
		app.CurrentScene()->SetAmbientLight(glm::vec3(0.1f));
		app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
		Luted = false;

	}

	//Specular lighting only
	if (InputEngine::GetKeyState(GLFW_KEY_3) == ButtonState::Pressed) {
		app.CurrentScene()->SetAmbientLight(glm::vec3(0.0f));
		app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
		Luted = false;
	}

	//Ambient + specular
	if (InputEngine::GetKeyState(GLFW_KEY_4) == ButtonState::Pressed) {
		app.CurrentScene()->SetAmbientLight(glm::vec3(0.1f));
		app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
		Luted = false;
	}

	//Ambient + Specular + [your custom effect / shader]
	if (InputEngine::GetKeyState(GLFW_KEY_5) == ButtonState::Pressed) {
		app.CurrentScene()->SetAmbientLight(glm::vec3(0.1f));
		app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/noir.CUBE"));
		Luted = true;
	}

	//Toggle diffuse warp/ramo
	if (InputEngine::GetKeyState(GLFW_KEY_6) == ButtonState::Pressed) {

	}

	//Toggle specular warp/ramp
	if (InputEngine::GetKeyState(GLFW_KEY_7) == ButtonState::Pressed) {

	}

	//Toggle Color Grading Warm
	if (InputEngine::GetKeyState(GLFW_KEY_8) == ButtonState::Pressed) {
		if (Luted == false)
		{
			app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/warm.CUBE"));
			Luted = true;
		}
		else
		{
			app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
			Luted = false;
		}

	}

	//Toggle Color Grading Cool
	if (InputEngine::GetKeyState(GLFW_KEY_9) == ButtonState::Pressed) {
		if (Luted == false)
		{
			app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE"));
			Luted = true;
		}
		else
		{
			app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
			Luted = false;
		}
	}

	//Toggle Color Grading Custom Effect
	if (InputEngine::GetKeyState(GLFW_KEY_0) == ButtonState::Pressed) {
		if (Luted == false)
		{
			app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/noir.CUBE"));
			Luted = true;
		}
		else
		{
			app.CurrentScene()->SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/plain.CUBE"));
			Luted = false;
		}
	}

	
}