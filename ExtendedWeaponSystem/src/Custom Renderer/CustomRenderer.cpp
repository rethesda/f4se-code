#include "Custom Renderer/CustomRenderer.h"

#include "Custom Renderer/ScopeGeometry.h"
#include "Util.h"
#include "WeaponHandlers.h"

#pragma region ScopeCamera

ScopeCamera::ScopeCamera() :
	TESCamera() {
	logInfo("ScopeCamera ctor Starting...");

	DefaultState* camDefaultState;
	ThermalState* camThermalState;
	NightVisionState* camNightVisionState;
	TESCameraState* oldCamState;

	//defaultState init
	camDefaultState = (DefaultState*)RE::malloc(sizeof(DefaultState));
	if (camDefaultState) {
		new (camDefaultState) DefaultState(*this, ScopeCameraStates::kDefault);
		cameraStates[ScopeCameraStates::kDefault].reset(camDefaultState);
		logInfo("ScopeCamera - Created ScopeCamera::DefaultState");
	} else {
		camDefaultState = nullptr;
		logError("ScopeCamera - ScopeCamera::DefaultState Creation FAILED");
	}
	//thermalState init
	camThermalState = (ThermalState*)RE::malloc(sizeof(ThermalState));
	if (camThermalState) {
		new (camThermalState) ThermalState(*this, ScopeCameraStates::kThermal);
		cameraStates[ScopeCameraStates::kThermal].reset(camThermalState);
		logInfo("ScopeCamera - Created ScopeCamera::ThermalState");
	} else {
		camThermalState = nullptr;
		logError("ScopeCamera - ScopeCamera::ThermalState Creation FAILED");
	}
	//nightVisionState init
	camNightVisionState = (NightVisionState*)RE::malloc(sizeof(NightVisionState));
	if (camNightVisionState) {
		new (camNightVisionState) NightVisionState(*this, ScopeCameraStates::kNightVision);
		cameraStates[ScopeCameraStates::kNightVision].reset(camNightVisionState);
		logInfo("ScopeCamera - Created ScopeCamera::NightVisionState");
	} else {
		camNightVisionState = nullptr;
		logError("ScopeCamera - ScopeCamera::NightVisionState Creation FAILED");
	}

	//oldState
	oldCamState = currentState.get();
	if (camDefaultState != oldCamState) {
		if (camDefaultState) {
			InterlockedIncrement(&camDefaultState->refCount);
		}
		currentState.reset(camDefaultState);
		if (oldCamState && !InterlockedDecrement(&oldCamState->refCount)) {
			oldCamState->~TESCameraState();
		}
	}

	//set state to default
	this->SetState(cameraStates[ScopeCameraStates::kDefault].get());
	logInfo("ScopeCamera ctor Completed.");
}

ScopeCamera::ScopeCamera(bool createDefault) :
	ScopeCamera() {
	if (createDefault) {
		CreateDefault3D();
	} else {
		camera = nullptr;
		renderPlane = nullptr;
		geometryDefault = false;
	}
}

ScopeCamera::~ScopeCamera() {
	TESCameraState* current;
	TESCameraState* state;

	NiNode* root;

	current = currentState.get();
	if (current) {
		current->End();
	}

	auto stateA = cameraStates;
	uint32_t total = ScopeCameraState::kTotal;
	do {
		state = stateA->get();
		if (stateA->get()) {
			stateA->~BSTSmartPointer();
			if (!InterlockedDecrement(&state->refCount)) {
				state->~TESCameraState();
			}
		}
		++stateA;
		--total;
	} while (total);

	//Geometry was created by us and needs to be destroyed
	if (geometryDefault) {
		if (!InterlockedDecrement(&camera->refCount)) {
			camera->DeleteThis();
		}
		if (!InterlockedDecrement(&renderPlane->refCount)) {
			renderPlane->DeleteThis();
		}
	}

	camera = nullptr;
	renderPlane = nullptr;

	root = cameraRoot.get();
	if (root) {
		if (!InterlockedDecrement(&root->refCount)) {
			root->DeleteThis();
		}
	}
}

void ScopeCamera::SetCameraRoot(NiNode* newRoot) {
	NiNode* currentRoot;

	currentRoot = cameraRoot.get();
	if (currentRoot != newRoot) {
		if (newRoot) {
			InterlockedIncrement(&newRoot->refCount);
		}
		cameraRoot = newRoot;
		if (currentRoot) {
			if (!InterlockedDecrement(&currentRoot->refCount)) {
				currentRoot->DeleteThis();
			}
		}
	}
}

void ScopeCamera::SetEnabled(bool bEnabled) {  //TODO
	enabled = bEnabled;
}

bool ScopeCamera::QCameraHasRenderPlane() {
	return renderPlane;
}

void ScopeCamera::CreateDefault3D() {
	NiCamera* cam;
	NiCamera* oldCam;
	NiCamera* newCam;
	NiCamera* currentCam;

	//cam = (NiCamera*)RE::malloc(0x1A0);
	cam = NiCamera::Create();
	if (cam) {
		newCam = cam;
		logInfo("ScopeCamera - Created NiCamera");
	} else {
		newCam = nullptr;
		logInfo("ScopeCamera - NiCamera Creation FAILED");
	}
	currentCam = camera;
	if (camera != newCam) {
		oldCam = camera;
		if (newCam) {
			InterlockedIncrement(&newCam->refCount);
		}
		currentCam = newCam;
		camera = newCam;
		if (oldCam) {
			if (!InterlockedDecrement(&oldCam->refCount)) {
				oldCam->DeleteThis();
			}
			currentCam = camera;
		}
	}

	BSGraphics::State* pGraphicsState = &BSGraphics::State::GetSingleton();
	float widthRatio = pGraphicsState->uiBackBufferWidth / pGraphicsState->uiBackBufferHeight;

	NiFrustum updatedFrustum{ NiFrustum(0) };
	updatedFrustum.nearPlane = 1.0F;
	updatedFrustum.farPlane = 10000.0F;
	updatedFrustum.leftPlane = widthRatio * -0.5F;
	updatedFrustum.rightPlane = widthRatio * 0.5F;
	updatedFrustum.bottomPlane = -0.5F;
	updatedFrustum.topPlane = 0.5F;
	updatedFrustum.ortho = false;
	camera->SetViewFrustrum(updatedFrustum);

	//more camera setup needed?

	NiNode* currentNode;
	NiNode* newNode;
	NiNode* node;

	node = (NiNode*)RE::malloc(0x140);
	//node = NiNode::Create();
	if (node) {
		new (node) NiNode(1);
		newNode = node;
		logInfo("ScopeCamera - Created NiNode");
	} else {
		newNode = nullptr;
		logError("ScopeCamera - NiNode Creation FAILED");
	}
	currentNode = cameraRoot.get();
	if (currentNode != newNode) {
		if (newNode) {
			InterlockedIncrement(&newNode->refCount);
		}
		cameraRoot = newNode;
		if (currentNode && !InterlockedDecrement(&currentNode->refCount)) {
			currentNode->DeleteThis();
		}
	}
	cameraRoot->AttachChild(camera, true);

	BSGeometry* geo;
	BSGeometry* newGeo;
	BSGeometry* currentGeo;

	geo = CreateScreenQuadShape(0, 1, 0, 1);
	//geo = (BSGeometry*)RE::malloc(0x160);
	if (geo) {
		//new (geo) BSGeometry();
		newGeo = geo;
		logInfo("ScopeCamera - Created BSTriShape");
	} else {
		newGeo = nullptr;
		logError("ScopeCamera - BSTriShape Creation FAILED");
	}
	currentGeo = renderPlane;
	if (currentGeo != newGeo) {
		if (newGeo) {
			InterlockedIncrement(&newGeo->refCount);
		}
		renderPlane = newGeo;
		if (currentGeo && !InterlockedDecrement(&currentGeo->refCount)) {
			currentGeo->DeleteThis();
		}
	}
	geometryDefault = true;

	//Needs aditional setup for the default render plane
	//lightingshaderproperty or effectshaderproperty needs to be made
	//position of render plane needs to be done
}

bool ScopeCamera::IsInDefaultMode() {
	TESCameraState* defaultState;

	defaultState = cameraStates[ScopeCameraState::kDefault].get();
	if (currentState.get() == defaultState) {
		return currentState.get() == defaultState;
	}
	return false;
}

bool ScopeCamera::IsInThermalMode() {
	TESCameraState* thermalState;

	thermalState = cameraStates[ScopeCameraState::kThermal].get();
	if (currentState.get() == thermalState) {
		return currentState.get() == thermalState;
	}
	return false;
}

bool ScopeCamera::IsInNightVisionMode() {
	TESCameraState* nightVisionState;

	nightVisionState = cameraStates[ScopeCameraState::kNightVision].get();
	if (currentState.get() == nightVisionState) {
		return currentState.get() == nightVisionState;
	}
	return false;
}

void ScopeCamera::Reset() {
	currentState->Begin();
}

void ScopeCamera::SetState(TESCameraState* newCameraState) {
	TESCameraState* lastState;
	TESCameraState* oldState;
	TESCameraState* newState;

	lastState = currentState.get();
	if (lastState) {
		lastState->End();
	}
	oldState = currentState.get();
	if (newCameraState != oldState) {
		if (newCameraState) {
			InterlockedIncrement(&newCameraState->refCount);
		}
		currentState.reset(newCameraState);
		if (oldState && !InterlockedDecrement(&oldState->refCount)) {
			oldState->~TESCameraState();
		}
	}
	newState = currentState.get();
	if (newState) {
		newState->Begin();
	}
}

void ScopeCamera::StartCorrectState() {
	if (WeaponHasNoSpecialScopes()) {
		logError("ScopeCamera::StartCorrectState() was called and could not find the correct state to start.");
		return;
	}
	if (weaponHasScopeThermal) {
		StartThermalState();
		goto FoundState;
	}
	if (weaponHasScopeNV) {
		StartNightVisionState();
		goto FoundState;
	}
	if (weaponHasScopePIP) {
		StartDefaultState();
		goto FoundState;
	}
FoundState:
	{
		UpdateCamera();
		return;
	}
}

void ScopeCamera::StartDefaultState() {
	TESCameraState* newState = cameraStates[ScopeCameraState::kDefault].get();
	SetState(newState);
}

void ScopeCamera::StartThermalState() {
	TESCameraState* newState = cameraStates[ScopeCameraState::kThermal].get();
	SetState(newState);
}

void ScopeCamera::StartNightVisionState() {
	TESCameraState* newState = cameraStates[ScopeCameraState::kNightVision].get();
	SetState(newState);
}

void ScopeCamera::Update3D() {
	BSGeometry* geom;
	BSGeometry* newGeom;
	BSGeometry* oldGeom;

	const BSFixedString geomName = "TextureLoader:0";

	logInfo("Looking for new camera and geometry...");

	geom = (BSGeometry*)GetByNameFromPlayer3D(geomName);
	if (geom) {
		logInfo("Found the geometry of the scope.");
		newGeom = geom;
	} else {
		logWarn("Could not find the geometry of the scope.");
		newGeom = nullptr;
	}
	oldGeom = renderPlane;
	if (renderPlane != newGeom) {
		if (newGeom) {
			renderPlane = newGeom;
		}

		if (oldGeom) {
		}
	}

	NiCamera* cam;
	NiCamera* newCam;
	NiCamera* oldCam;

	const BSFixedString camName = "ScopePOV";

	cam = (NiCamera*)GetByNameFromPlayer3D(camName);
	if (cam) {
		logInfo("Found the scope camera.");
		newCam = cam;
	} else {
		logWarn("Could not find the camera of the scope.");
		newCam = nullptr;
	}
	oldCam = camera;
	if (camera != newCam) {
		if (newCam) {
			camera = newCam;
			SetCameraRoot(camera->parent);
		}

		if (oldCam) {
		}
	}

	StartCorrectState();

	//TODO: add actor value or something similar to set what the FOV should be on the camera of each scope
	BSShaderUtil::SetSceneGraphCameraFOV(Main::GetWorldSceneGraph(), (90.0 / 4.0), 0, camera, 1);  //TEMP. Right now I just have it as 4x zoom
}

void ScopeCamera::UpdateCamera() {
	if (!enabled) {
		return;
	}

	if (!QCameraHasRenderPlane()) {
		Update3D();
	}

	TESCameraState* pCurrentState = currentState.get();
	if (pCurrentState == cameraStates[ScopeCameraState::kDefault].get()) {
		((ScopeCamera::DefaultState*)pCurrentState)->Update(currentState);
	}
	if (pCurrentState == cameraStates[ScopeCameraState::kThermal].get()) {
		((ScopeCamera::ThermalState*)pCurrentState)->Update(currentState);
	}
	if (pCurrentState == cameraStates[ScopeCameraState::kNightVision].get()) {
		((ScopeCamera::NightVisionState*)pCurrentState)->Update(currentState);
	}
}

ScopeCamera::DefaultState::DefaultState(TESCamera& cam, std::uint32_t ID) :
	TESCameraState(cam, ID) {
	logInfo("ScopeCamera::State ctor Starting...");
	refCount = 0;
	camera = &cam;
	id = ID;

	initialRotation.w = 0.0F;
	initialRotation.x = 0.0F;
	initialRotation.y = 0.0F;
	initialRotation.z = 0.0F;
	initialPosition = NiPoint3::ZERO;
	rotation.w = 0.0F;
	rotation.x = 0.0F;
	rotation.y = 0.0F;
	rotation.z = 0.0F;
	translation = NiPoint3::ZERO;
	zoom = 1.0F;
}

ScopeCamera::DefaultState::~DefaultState() {
}

bool ScopeCamera::DefaultState::ShouldHandleEvent(const InputEvent* inputEvent) {  //TODO
	return false;                                                                  //TEMP
}

void ScopeCamera::DefaultState::HandleThumbstickEvent(const ThumbstickEvent* inputEvent) {
}

void ScopeCamera::DefaultState::HandleCursorMoveEvent(const CursorMoveEvent* inputEvent) {
}

void ScopeCamera::DefaultState::HandleMouseMoveEvent(const MouseMoveEvent* inputEvent) {
}

void ScopeCamera::DefaultState::HandleButtonEvent(const ButtonEvent* inputEvent) {  //TODO
}

void ScopeCamera::DefaultState::Begin() {  //TODO: Add new members
	translation = NiPoint3::ZERO;
	zoom = 1.0F;
}

void ScopeCamera::DefaultState::End() {  //TODO
}

void ScopeCamera::DefaultState::Update(BSTSmartPointer<TESCameraState>& a_nextState) {
	NiPoint3 updatedTranslation;
	NiQuaternion updatedQuatRotation;
	NiMatrix3 updatedRotation;

	NiPointer<NiNode> spCameraRoot = nullptr;
	NiNode* pCameraRoot = nullptr;

	NiUpdateData updateData{ NiUpdateData(0.0F, 0) };

	GetTranslation(updatedTranslation);
	GetRotation(updatedQuatRotation);

	if (camera->GetCameraRoot(spCameraRoot)) {
		updatedQuatRotation.ToRotation(updatedRotation);

		pCameraRoot->local.rotate.entry[0] = updatedRotation.entry[0];
		pCameraRoot->local.rotate.entry[1] = updatedRotation.entry[1];
		pCameraRoot->local.rotate.entry[2] = updatedRotation.entry[2];
		pCameraRoot->local.translate = updatedTranslation;
		pCameraRoot->Update(updateData);
	}

	float newZoom = camera->zoomInput + zoom;
	if (newZoom <= 1.0F) {
		if (newZoom < 0.0F) {
			newZoom = 0.0F;
		}
	} else {
		newZoom = 1.0F;
	}
	zoom = newZoom;

	//delete the default created node of the nismartpointer???
	NiNode* oldCameraRoot = spCameraRoot.get();
	if (spCameraRoot.get()) {
		if (!InterlockedDecrement(&spCameraRoot->refCount)) {
			oldCameraRoot->DeleteThis();
		}
	}
}

void ScopeCamera::DefaultState::GetRotation(NiQuaternion& a_rotation) const {
	a_rotation = rotation;
}

void ScopeCamera::DefaultState::GetTranslation(NiPoint3& a_translation) const {
	a_translation = translation;
}

void ScopeCamera::DefaultState::SaveGame(BGSSaveFormBuffer* a_saveGameBuffer) {
}

void ScopeCamera::DefaultState::LoadGame(BGSLoadFormBuffer* a_loadGameBuffer) {
}

void ScopeCamera::DefaultState::Revert(BGSLoadFormBuffer* a_loadGameBuffer) {
}

void ScopeCamera::DefaultState::SetInitialRotation(NiQuaternion& newRotation) {
	initialRotation = newRotation;
}

void ScopeCamera::DefaultState::SetInitialPosition(NiPoint3& newPos) {
	initialPosition = newPos;
}

void ScopeCamera::DefaultState::SetRotation(NiQuaternion& newRotation) {
	rotation = newRotation;
}

void ScopeCamera::DefaultState::SetTranslation(NiPoint3& newPos) {
	translation = newPos;
}

void ScopeCamera::DefaultState::SetZoom(float newZoom) {
	zoom = newZoom;
}

ScopeCamera::ThermalState::ThermalState(TESCamera& cam, std::uint32_t ID) :
	DefaultState(cam, ID) {
	logInfo("ScopeCamera::ThermalState ctor Completed.");
}

ScopeCamera::ThermalState::~ThermalState() {
}

bool ScopeCamera::ThermalState::ShouldHandleEvent(const InputEvent* inputEvent) {
	return false;
}

void ScopeCamera::ThermalState::HandleThumbstickEvent(const ThumbstickEvent* inputEvent) {
}

void ScopeCamera::ThermalState::HandleCursorMoveEvent(const CursorMoveEvent* inputEvent) {
}

void ScopeCamera::ThermalState::HandleMouseMoveEvent(const MouseMoveEvent* inputEvent) {
}

void ScopeCamera::ThermalState::HandleButtonEvent(const ButtonEvent* inputEvent) {
}

void ScopeCamera::ThermalState::Begin() {
}

void ScopeCamera::ThermalState::End() {
}

void ScopeCamera::ThermalState::Update(BSTSmartPointer<TESCameraState>& a_nextState) {
}

ScopeCamera::NightVisionState::NightVisionState(TESCamera& cam, std::uint32_t ID) :
	DefaultState(cam, ID) {
	logInfo("ScopeCamera::NightVisionState ctor Completed.");
}

ScopeCamera::NightVisionState::~NightVisionState() {
}

bool ScopeCamera::NightVisionState::ShouldHandleEvent(const InputEvent* inputEvent) {  //TODO
	return false;
}

void ScopeCamera::NightVisionState::HandleThumbstickEvent(const ThumbstickEvent* inputEvent) {
}

void ScopeCamera::NightVisionState::HandleCursorMoveEvent(const CursorMoveEvent* inputEvent) {
}

void ScopeCamera::NightVisionState::HandleMouseMoveEvent(const MouseMoveEvent* inputEvent) {
}

void ScopeCamera::NightVisionState::HandleButtonEvent(const ButtonEvent* inputEvent) {
}

void ScopeCamera::NightVisionState::Begin() {
}

void ScopeCamera::NightVisionState::End() {
}

void ScopeCamera::NightVisionState::Update(BSTSmartPointer<TESCameraState>& a_nextState) {
}

#pragma endregion

#pragma region ScopeRenderer

ScopeRenderer::ScopeRenderer() {
	logInfo("ScopeRenderer ctor Starting...");

	BSShaderAccumulator* shaderAccum;
	BSShaderAccumulator* newShaderAccum;
	BSShaderAccumulator* oldShaderAccum;

	logInfo("ScopeRenderer - Creating BSCullingProcess...");
	pScopeCullingProc = (BSCullingProcess*)RE::malloc(0x1A0);
	if (pScopeCullingProc) {
		new (pScopeCullingProc) BSCullingProcess(0);
	} else {
		pScopeCullingProc = nullptr;
		logError("ScopeRenderer - BSCullingProcess Creation FAILED");
	}

	logInfo("ScopeRenderer - Creating ScopeCamera...");
	pRendererCamera = (ScopeCamera*)RE::malloc(sizeof(ScopeCamera));
	if (pRendererCamera) {
		new (pRendererCamera) ScopeCamera(false);
	} else {
		logError("ScopeRenderer - ScopeCamera Creation FAILED");
	}

	logInfo("ScopeRenderer - Creating ImageSpaceShaderParam...");
	pShaderParams = (ImageSpaceShaderParam*)RE::malloc(0x90);
	if (pShaderParams) {
		new (pShaderParams) ImageSpaceShaderParam();
	} else {
		logError("ScopeRenderer - ImageSpaceShaderParam Creation FAILED");
		pShaderParams = &BSImagespaceShader::GetDefaultParam();
	}

	shaderAccum = (BSShaderAccumulator*)RE::malloc(0x590);
	if (shaderAccum) {
		new (shaderAccum) BSShaderAccumulator();
		newShaderAccum = shaderAccum;
		logInfo("ScopeRenderer - Created BSShaderAccumulator");
	} else {
		newShaderAccum = nullptr;
		logError("ScopeRenderer - BSShaderAccumulator Creation FAILED");
	}
	oldShaderAccum = pScopeAccumulator;
	if ((oldShaderAccum != newShaderAccum) || (!pScopeAccumulator)) {
		if (newShaderAccum) {
			InterlockedIncrement(&newShaderAccum->refCount);
		}
		pScopeAccumulator = newShaderAccum;
		if (oldShaderAccum && !InterlockedDecrement(&oldShaderAccum->refCount)) {
			oldShaderAccum->DeleteThis();
		}
	}
	pScopeAccumulator->ZPrePass = true;
	pScopeAccumulator->activeShadowSceneNode = BSShaderManager::State::GetSingleton().pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD];
	pScopeAccumulator->QSilhouetteColor = NiColorA::WHITE;
	pScopeCullingProc->SetAccumulator(pScopeAccumulator);
	pScopeCullingProc->kCullMode = BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS;
	pScopeCullingProc->bCameraRelatedUpdates = false;
	//pShaderParams->ResizeConstantGroup(0, 1); //CTD
	renderTarget = 19;

	logInfo("ScopeRenderer ctor Completed.");
}

ScopeRenderer::~ScopeRenderer() {
	logInfo("ScopeRenderer dtor Starting...");
	BSShaderAccumulator* shaderAccum;
	NiCamera* cam;
	TESCameraState* state;

	pShaderParams->~ImageSpaceShaderParam();
	shaderAccum = pScopeAccumulator;
	if (shaderAccum && !InterlockedDecrement(&shaderAccum->refCount)) {
		shaderAccum->DeleteThis();
	}
	cam = pRendererCamera->camera;
	if (cam && !InterlockedDecrement(&cam->refCount)) {
		cam->DeleteThis();
	}
	state = pRendererCamera->currentState.get();
	if (state && !InterlockedDecrement(&state->refCount)) {
		state->~TESCameraState();
	}
	pRendererCamera->~ScopeCamera();
	pScopeCullingProc->~BSCullingProcess();
	logInfo("ScopeRenderer dtor Completed.");
}

NiTexture* ScopeRenderer::Render() {
	logInfo("ScopeRenderer::Render() Starting...");
	BSGraphics::State* pGraphicsState = &BSGraphics::State::GetSingleton();
	BSGraphics::RenderTargetManager* pTargetManager = &BSGraphics::RenderTargetManager::GetSingleton();
	BSGraphics::Renderer* pRenderData = &BSGraphics::Renderer::GetSingleton();
	ImageSpaceManager* pImageSpaceManager = ImageSpaceManager::GetSingleton();
	BSShaderManager::State* pShaderState = &BSShaderManager::State::GetSingleton();
	NiCamera* shaderCam = BSShaderManager::GetCamera();

	ShadowSceneNode* pWorldSSN = pShaderState->pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD];
	NiAVObject* objectLODRoot = pShaderState->pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]->children[3].get();
	NiAVObject* pObjectLODRoot = objectLODRoot;
	if (objectLODRoot) {
		InterlockedIncrement(&objectLODRoot->refCount);
	}
	bool objectLODRootCull = objectLODRoot->flags.GetBit(NiAVObject::APP_CULLED_MASK);
	objectLODRoot->SetAppCulled(true);
	NiNode* grassNode = BGSGrassManager::GetSingleton()->grassNode;
	NiNode* pGrassNode = grassNode;
	bool grassCull = grassNode->flags.GetBit(NiAVObject::APP_CULLED_MASK);
	bool grassCullFlag = grassCull;
	grassNode->SetAppCulled(true);
	BSDistantObjectInstanceRenderer::QInstance().enabled = false;

	memcpy(BSShaderManager::GetCamera(), pRendererCamera->camera, 0x1A0);

	pGraphicsState->SetCameraData(pRendererCamera->camera, false, 0.0F, 1.0F);
	pScopeCullingProc->SetAccumulator(pScopeAccumulator);
	pScopeCullingProc->kCullMode = BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS;
	pScopeCullingProc->bCameraRelatedUpdates = false;
	pScopeCullingProc->pkCamera = pRendererCamera->camera;
	pRendererCamera->Update();

	bool lightUpdateLast = pWorldSSN->DisableLightUpdate;
	pWorldSSN->DisableLightUpdate = true;
	bool lightUpdate = lightUpdateLast;

	REL::Relocation<bool*> BSFadeNode_bFadeEnabled{ REL::ID(1220201) };
	REL::Relocation<bool*> BSFadeNode_bDrawFadingEnabled{ REL::ID(1445134) };
	REL::Relocation<int*> BSFadeNode_iFadeEnableCounter{ REL::ID(580186) };

	bool fadeEnabledFlag = *BSFadeNode_bFadeEnabled;
	memset(&*BSFadeNode_bFadeEnabled, 0, sizeof(bool));
	memset(&*BSFadeNode_bDrawFadingEnabled, 0, sizeof(bool));
	memset(&*BSFadeNode_iFadeEnableCounter, 0, sizeof(int));
	bool fadeEnabled = fadeEnabledFlag;

	pRenderData->SetClearColor(0.2F, 0.2F, 0.2F, 1.0F);

	//water here

	pScopeAccumulator->renderMode = BSShaderManager::etRenderMode::BSSM_RENDER_SCREEN_SPLATTER;
	pScopeAccumulator->eyePosition = pRendererCamera->camera->world.translate;
	pRenderData->ResetZPrePass();

	//here is where we would go through the array of cells for a top down render like Local Map Renderer

	BSPortalGraphEntry* camPortalEntry = Main::GetSingleton()->GetCameraPortalGraphEntry();
	if (camPortalEntry) {
		BSPortalGraph* camPortalGraph = camPortalEntry->PortalGraph;
		if (camPortalGraph) {
			BSShaderUtil::AccumulateSceneArray(pRendererCamera->camera, &camPortalGraph->AlwaysRenderArray, *pScopeCullingProc, false);
		}
	}

	NiAVObject* portalSharedNode;
	if (pWorldSSN->children.size() > 9) {
		portalSharedNode = pWorldSSN->children[9].get();
	} else {
		portalSharedNode = nullptr;
	}
	BSShaderUtil::AccumulateScene(pRendererCamera->camera, portalSharedNode, *pScopeCullingProc, false);

	NiAVObject* multiBoundNode;
	if (pWorldSSN->children.size() > 8) {
		multiBoundNode = pWorldSSN->children[8].get();
	} else {
		multiBoundNode = nullptr;
	}
	BSShaderUtil::AccumulateScene(pRendererCamera->camera, multiBoundNode, *pScopeCullingProc, false);
	pShaderState->cSceneGraph = BSShaderManager::BSSM_SSN_WORLD;
	pShaderState->pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]->ProcessQueuedLights(pScopeCullingProc);
	pRenderData->ResetState();

	uint32_t renderTargetID = 24;  //RENDER_TARGET_HUDGLASS
	uint32_t depthTargetID = 1;    //DEPTH_STENCIL_TARGET_MAIN
	uint32_t effectID = 162;       //kBSVatsTarget
	//logInfo("Acquiring Targets...");	//Need to do more research for if we need to acquire targets or not
	//pTargetManager->AcquireDepthStencil(depthTargetID);
	//pTargetManager->AcquireRenderTarget(renderTargetID);
	logInfo("Setting Targets...");
	RenderScopeScene(pRendererCamera->camera, pScopeAccumulator, renderTargetID, depthTargetID);

	pTargetManager->SetCurrentRenderTarget(0, 2, BSGraphics::SetRenderTargetMode::SRTM_RESTORE);  //RENDER_TARGET_MAIN_COPY
	pTargetManager->SetCurrentRenderTarget(1, -1, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);   //RENDER_TARGET_NONE
	pTargetManager->SetCurrentDepthStencilTarget(depthTargetID, BSGraphics::SetRenderTargetMode::SRTM_RESTORE, 0, false);
	pTargetManager->SetCurrentDepthStencilTarget(0, BSGraphics::SetRenderTargetMode::SRTM_RESTORE, 0, false);

	//pShaderParams->SetPixelConstant(0,
	//	1.0F / pTargetManager->pRenderTargetDataA[renderTargetID].uiWidth,
	//	1.0F / pTargetManager->pRenderTargetDataA[renderTargetID].uiHeight,
	//	0.0F,
	//	0.0F);

	const BSFixedString strScope("ScopeTexture");
	NiTexture* renderedTexture = renderedTexture->CreateEmpty(strScope, false, false);

	pTargetManager->SetTextureDepth(1, depthTargetID);
	pTargetManager->SetTextureRenderTarget(2, renderTargetID, false);
	pImageSpaceManager->effectArray[effectID].UseDynamicResolution = false;
	pImageSpaceManager->RenderEffectHelper_Tex_1((ImageSpaceManager::ImageSpaceEffectEnum)effectID, renderedTexture, renderTargetID, pShaderParams);
	pImageSpaceManager->RenderEffect_1((ImageSpaceManager::ImageSpaceEffectEnum)effectID, renderTargetID, pShaderParams);
	pTargetManager->SetCurrentRenderTarget(0, 1, BSGraphics::SetRenderTargetMode::SRTM_RESTORE);  //RENDER_TARGET_MAIN

	renderedTexture->SetRendererTexture(pTargetManager->SaveRenderTargetToTexture(renderTargetID, false, false, BSGraphics::Usage::USAGE_DEFAULT));

	logInfo("Releasing Targets...");
	pTargetManager->ReleaseDepthStencil(depthTargetID);
	pTargetManager->ReleaseRenderTarget(renderTargetID);

	memcpy(BSShaderManager::GetCamera(), shaderCam, 0x1A0);
	pGraphicsState->SetCameraData(shaderCam, false, 0.0F, 1.0F);
	pWorldSSN->DisableLightUpdate = lightUpdate;

	memset(&*BSFadeNode_iFadeEnableCounter, 0, sizeof(int));
	memset(&*BSFadeNode_bFadeEnabled, fadeEnabled, sizeof(bool));
	memset(&*BSFadeNode_bDrawFadingEnabled, fadeEnabled, sizeof(bool));

	//water here

	pScopeAccumulator->ClearActivePasses(false);
	pScopeAccumulator->ClearGroupPasses(5, false);
	BSDistantObjectInstanceRenderer::QInstance().enabled = true;

	NiAVObject* pOldGrassNode = pGrassNode;
	pGrassNode->SetAppCulled(grassCull);
	if (!InterlockedDecrement(&pOldGrassNode->refCount)) {
		pOldGrassNode->DeleteThis();
	}

	objectLODRoot->SetAppCulled(objectLODRootCull);
	if (!InterlockedDecrement(&objectLODRoot->refCount)) {
		objectLODRoot->DeleteThis();
	}
	return renderedTexture;
}

NiTexture* ScopeRenderer::RenderSimple() {
	logInfo("ScopeRenderer::RenderSimple() Starting...");
	BSGraphics::State* pGraphicsState = &BSGraphics::State::GetSingleton();
	BSGraphics::RenderTargetManager* pTargetManager = &BSGraphics::RenderTargetManager::GetSingleton();
	BSGraphics::Renderer* pRenderData = &BSGraphics::Renderer::GetSingleton();
	ImageSpaceManager* pImageSpaceManager = ImageSpaceManager::GetSingleton();
	BSShaderManager::State* pShaderState = &BSShaderManager::State::GetSingleton();
	ShadowSceneNode* pWorldSSN = pShaderState->pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD];
	NiCamera* pShaderCam = BSShaderManager::GetCamera();

	uint32_t renderTargetID = 24;  //RENDER_TARGET_HUDGLASS
	uint32_t depthTargetID = 1;    //DEPTH_STENCIL_TARGET_MAIN
	uint32_t effectID = 162;       //kBSVatsTarget

	pRenderData->ResetState();
	pGraphicsState->SetCameraData(pRendererCamera->camera, false, 0.0F, 1.0F);
	pScopeCullingProc->SetAccumulator(pScopeAccumulator);
	pScopeCullingProc->kCullMode = BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS;
	pScopeCullingProc->bCameraRelatedUpdates = false;
	pScopeCullingProc->pkCamera = pRendererCamera->camera;
	pRendererCamera->Update();

	pRenderData->SetClearColor(0.2F, 0.2F, 0.2F, 1.0F);

	pScopeAccumulator->activeShadowSceneNode = pShaderState->pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD];
	pScopeAccumulator->renderMode = BSShaderManager::etRenderMode::BSSM_RENDER_VATS_MASK;
	pScopeAccumulator->eyePosition = pRendererCamera->camera->world.translate;

	NiAVObject* multiBoundNode;
	if (pWorldSSN->children.size() > 8) {
		multiBoundNode = pWorldSSN->children[8].get();
	} else {
		multiBoundNode = nullptr;
	}
	BSShaderUtil::AccumulateScene(pRendererCamera->camera, multiBoundNode, *pScopeCullingProc, false);
	//BSShaderUtil::RenderScene(pRendererCamera->camera, pScopeAccumulator, false);

	pTargetManager->SetCurrentDepthStencilTarget(depthTargetID, BSGraphics::SetRenderTargetMode::SRTM_FORCE_COPY_RESTORE, 0, false);
	pTargetManager->SetCurrentRenderTarget(1, 1, BSGraphics::SetRenderTargetMode::SRTM_RESTORE);
	pTargetManager->SetCurrentRenderTarget(0, renderTargetID, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);
	pTargetManager->SetCurrentViewportForceToRenderTargetDimensions();

	const BSFixedString strScope("ScopeTexture");
	NiTexture* renderedTexture = renderedTexture->CreateEmpty(strScope, false, false);

	pTargetManager->SetTextureDepth(1, depthTargetID);
	pTargetManager->SetTextureRenderTarget(1, renderTargetID, false);
	//pImageSpaceManager->RenderEffect_1((ImageSpaceManager::ImageSpaceEffectEnum)effectID, renderTargetID, nullptr);
	//pImageSpaceManager->RenderEffectHelper_2((ImageSpaceManager::ImageSpaceEffectEnum)effectID, renderTargetID, 2, nullptr);
	renderedTexture->SetRendererTexture(pTargetManager->SaveRenderTargetToTexture(renderTargetID, false, false, BSGraphics::Usage::USAGE_DEFAULT));
	pImageSpaceManager->RenderEffectHelper_Tex_1((ImageSpaceManager::ImageSpaceEffectEnum)effectID, renderedTexture, renderTarget, nullptr);

	pRenderData->RestorePreviousClearColor();
	pTargetManager->SetCurrentRenderTarget(0, -1, BSGraphics::SetRenderTargetMode::SRTM_RESTORE);
	pGraphicsState->SetCameraData(pShaderCam, false, 0.0F, 1.0F);
	

	return renderedTexture;
}

#pragma endregion

void ScopeRenderer::RenderScopeScene(NiCamera* a_camera, BSShaderAccumulator* a_shaderAccumulator, uint32_t a_renderTarget, uint32_t a_depthTarget) {
	BSGraphics::State* pGraphicsState = &BSGraphics::State::GetSingleton();
	BSGraphics::RenderTargetManager* pTargetManager = &BSGraphics::RenderTargetManager::GetSingleton();
	BSGraphics::Renderer* pRenderData = &BSGraphics::Renderer::GetSingleton();
	BSShaderManager::State* pShaderState = &BSShaderManager::State::GetSingleton();

	pTargetManager->SetCurrentDepthStencilTarget(a_depthTarget, BSGraphics::SetRenderTargetMode::SRTM_FORCE_COPY_RESTORE, 0, false);
	pTargetManager->SetCurrentRenderTarget(0, a_renderTarget, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);  //BASEMAP
	pTargetManager->SetCurrentRenderTarget(1, -1, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);              //NORMAL
	pTargetManager->SetCurrentRenderTarget(2, -1, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);              //ENVMAP
	pTargetManager->SetCurrentRenderTarget(3, -1, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);              //
	if (pShaderState->bDeferredRGBEmit) {
		pTargetManager->SetCurrentRenderTarget(4, -1, BSGraphics::SetRenderTargetMode::SRTM_CLEAR);  //GLOWMAP
	}
	pTargetManager->SetCurrentRenderTarget(5, -1, BSGraphics::SetRenderTargetMode::SRTM_RESTORE);  //
	pTargetManager->SetCurrentViewportForceToRenderTargetDimensions();
	pRenderData->SetClearColor(0.0F, 0.0F, 0.0F, 0.0F);
	pRenderData->ClearColor();
	pRenderData->Flush();
	pGraphicsState->SetCameraData(a_camera, false, 0.0F, 1.0F);
	pRenderData->DoZPrePass(nullptr, nullptr, 0.0F, 1.0F, 0.0F, 1.0F);
	a_shaderAccumulator->RenderOpaqueDecals();
	a_shaderAccumulator->RenderBatches(4, false, -1);
	a_shaderAccumulator->RenderBlendedDecals();
	pRenderData->Flush();
	pRenderData->SetClearColor(0.2F, 0.2F, 0.2F, 1.0F);
	a_shaderAccumulator->ClearEffectPasses();
	a_shaderAccumulator->ClearActivePasses(false);
}

#pragma region nsScope
namespace nsScope {
	ScopeRenderer* scopeRenderer = nullptr;
	bool initialized = false;

	void CreateRenderer() {
		logInfo("ScopeRenderer Creation Starting...");

		//there is already an exsisting renderer
		if (scopeRenderer) {
			logInfo("nsScope::CreateRenderer() was called but there was already a renderer in place");
			return;
		}

		//if renderer is null
		if (!scopeRenderer) {
			scopeRenderer = InitRenderer();
		}
		initialized = true;

		logInfo("ScopeRenderer Creation Complete.");
		logger::info(FMT_STRING("ScopeRenderer created at {:p}"), fmt::ptr(scopeRenderer));
	}

	void DestroyRenderer() {
		if (!scopeRenderer) {
			//logWarn("nsScope::DestroyRenderer() Was called but renderer is already nullptr");
			return;
		}
		logInfo("ScopeRenderer Destroy Starting...");
		scopeRenderer->~ScopeRenderer();
		scopeRenderer = nullptr;
		initialized = false;
		logInfo("ScopeRenderer Destroy Complete.");
	}

	ScopeRenderer* InitRenderer() {
		logInfo("ScopeRenderer Init Starting...");

		ScopeRenderer* renderer;
		ScopeRenderer* newRenderer;

		//allocate our renderer
		renderer = (ScopeRenderer*)RE::malloc(sizeof(ScopeRenderer));
		logInfo("ScopeRenderer Allocated...");
		//if allocated succesful
		if (renderer) {
			new (renderer) ScopeRenderer();
			newRenderer = renderer;
		} else {
			newRenderer = nullptr;
		}

		logInfo("ScopeRenderer Init Complete.");
		return newRenderer;
	}

	void Render() {
		if (!scopeRenderer) {
			logWarn("nsScope::Render() was called while scopeRenderer was nullptr");
			return;
		}
		NiTexture* renderedTexture;

		//renderedTexture = scopeRenderer->Render();
		renderedTexture = scopeRenderer->RenderSimple();
		if (renderedTexture) {
			if (scopeRenderer->pRendererCamera->renderPlane->shaderProperty.get()->Type() != NiProperty::SHADE) {
				//TODO: Add a creation of a shader property to the geometry for if the shaderProperty of the current geometry is nullptr or invalid
			}
			BSShaderProperty* shaderProperty = scopeRenderer->pRendererCamera->renderPlane->QShaderProperty();
			if (shaderProperty->GetMaterialType() == BSShaderMaterial::BSMATERIAL_TYPE_EFFECT) {
				BSEffectShaderProperty* effectShaderProperty;
				BSEffectShaderMaterial* effectShaderMaterial;
				effectShaderProperty = (BSEffectShaderProperty*)shaderProperty;
				if (shaderProperty) {
					effectShaderMaterial = effectShaderProperty->GetEffectShaderMaterial();
					effectShaderMaterial->SetBaseTexture(renderedTexture);
					effectShaderMaterial->fBaseColorScale = 1.0F;
					effectShaderMaterial->kBaseColor = NiColorA::WHITE;
				}
			}
			if (shaderProperty->GetMaterialType() == BSShaderMaterial::BSMATERIAL_TYPE_LIGHTING) {
				BSLightingShaderProperty* lightingShaderProperty;
				BSLightingShaderMaterialBase* lightingShaderMaterial;
				lightingShaderProperty = (BSLightingShaderProperty*)(shaderProperty);
				if (shaderProperty) {
					lightingShaderMaterial = lightingShaderProperty->GetLightingShaderMaterial();
					lightingShaderMaterial->spDiffuseTexture.reset(renderedTexture);
				}
			}
		}
	}
}

#pragma endregion
