#include "NISystem.h"

namespace {

void printError(HWND hWnd,const char *name, XnStatus nRetVal)
{
	char moji[256];
	sprintf_s(moji,sizeof(moji),"%s failed: %s\n", name, xnGetStatusString(nRetVal));
	MessageBoxA(hWnd,moji,"error",MB_OK);
}

UINT getClosestPowerOfTwo(UINT n)
{
	unsigned int m = 2;
	while(m < n) m<<=1;

	return m;
}

}


// callbacks
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& g, XnUserID id, void* cookie) {
	static_cast< NISystem* >(cookie)->OnNewUser(g,id); }
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& g,XnUserID id, void* cookie) {
	static_cast< NISystem* >(cookie)->OnLostUser(g,id); }
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& cap, XnUserID id, void* cookie) {
	static_cast< NISystem* >(cookie)->OnCalibrationStart(cap,id); }
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& cap, XnUserID id, XnBool b, void* cookie) {
	static_cast< NISystem* >(cookie)->OnCalibrationEnd(cap,id,b); }
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& cap, const XnChar* pos, XnUserID id, void* cookie) {
	static_cast< NISystem* >(cookie)->OnPoseDetected(cap,pos,id); }



NISystem::NISystem()
: m_view(this)
{
	m_bNeedPose = false;
	m_strPose[0] = '\0';
	m_pDepthTex = NULL;

	m_Users.resize(15);
	for (size_t i=0; i<m_Users.size(); ++i) {
		m_Users[i] = new NIUserData();
	}
}

NISystem::~NISystem()
{
	Clean();
	for (size_t i=0; i<m_Users.size(); ++i) {
		delete m_Users[i];
	}
	m_Users.resize(0);
}

bool NISystem::OnAttach(HINSTANCE h)
{
	if (! m_view.onAttach(h)) { return false; }
	return true;
}

bool NISystem::OnDetach(HINSTANCE h)
{
	if (! m_view.onDetach(h)) { return false; }
	Clean();
	return true;
}

bool NISystem::Setup(HWND hWnd, bool EngFlag, LPDIRECT3DDEVICE9 lpDevice, const char* file)
{
	FILE *fp;
	if((fp=fopen("Data\\SamplesConfig.xml","r"))!=NULL){
		fclose( fp );
	}else{
		if(EngFlag) MessageBox(hWnd,L"SamplesConfig.xml cannot find in Data folder.\n\nPlease download DxOpenNI and get SamplesConfig.xml from in it\nand put it into \"Data\" folder of MMD.",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"SamplesConfig.xmlがDataフォルダ内にありません。\n\nDxOpenNIをダウンロードしてその中からSamplesConfig.xmlをMMDのDataフォルダに入れて下さい。",L"Kinect",MB_OK);
		return false;
	}

	for (size_t i=0; i<m_Users.size(); ++i) {
		m_Users[i]->Setup();
	}

	XnStatus nRetVal = m_Context.InitFromXmlFile("Data\\SamplesConfig.xml");
	if(nRetVal != XN_STATUS_OK){
		printError(hWnd,"InitFromXmlFile", nRetVal);
		if(EngFlag) MessageBox(hWnd,L"Cannot find Kinect sensor",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectが接続されていません",L"Kinect",MB_OK);
		return false;
	}

	nRetVal = m_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, m_DepthGenerator);
	if(nRetVal != XN_STATUS_OK){
		printError(hWnd,"FindExistingNode", nRetVal);
		if(EngFlag) MessageBox(hWnd,L"Cannot find Kinect Depth generator",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectの深度センサーを認識できません",L"Kinect",MB_OK);
		Clean();
		return false;
	}

	nRetVal = m_Context.FindExistingNode(XN_NODE_TYPE_USER, m_UserGenerator);
	if(nRetVal != XN_STATUS_OK){
		nRetVal = m_UserGenerator.Create(m_Context);
		if(nRetVal != XN_STATUS_OK){
			printError(hWnd,"g_UserGenerator.Create", nRetVal);
			if(EngFlag) MessageBox(hWnd,L"Cannot find Kinect User generator",L"Kinect",MB_OK);
			else		MessageBox(hWnd,L"Kinectのユーザーセンサーを認識できません",L"Kinect",MB_OK);
			Clean();
			return false;
		}
	}

	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;
	if(!m_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)){
		if(EngFlag) MessageBox(hWnd,L"Supplied user generator doesn't support skeleton",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectのユーザーセンサーがボーン構造をサポートしていません",L"Kinect",MB_OK);
		Clean();
		return false;
	}
	m_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, this, hUserCallbacks);
	m_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, this, hCalibrationCallbacks);

	m_bNeedPose = false;
	m_strPose[0] = '\0';
	if(m_UserGenerator.GetSkeletonCap().NeedPoseForCalibration()){
		m_bNeedPose = true;
		if(!m_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)){
			if(EngFlag) MessageBox(hWnd,L"Pose required, but not supported",L"Kinect",MB_OK);
			else		MessageBox(hWnd,L"Kinectがキャリブレーションポーズをサポートしていません",L"Kinect",MB_OK);
			Clean();
			return false;
		}
		m_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, this, hPoseCallbacks);
		m_UserGenerator.GetSkeletonCap().GetCalibrationPose(m_strPose);
	}
	m_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	nRetVal = m_Context.StartGeneratingAll();
	if(nRetVal != XN_STATUS_OK){
		printError(hWnd,"StartGeneratingAll", nRetVal);
		if(EngFlag) MessageBox(hWnd,L"Cannot start Kinect generating",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectセンサーを始動できません",L"Kinect",MB_OK);
		Clean();
		return false;
	}

	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	m_DepthGenerator.GetMetaData(depthMD);
	m_Context.WaitAndUpdateAll();
	m_DepthGenerator.GetMetaData(depthMD);
	m_UserGenerator.GetUserPixels(0, sceneMD);

	int x=depthMD.XRes();
	int y=depthMD.YRes();
	m_texWidth =  getClosestPowerOfTwo(x/4);
	m_texHeight = getClosestPowerOfTwo(y/4);
	m_pDepthTex = NULL;

	if(FAILED(lpDevice->CreateTexture(m_texWidth,m_texHeight,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&m_pDepthTex,NULL))){
		if(EngFlag) MessageBox(hWnd,L"cannot make DepthTex",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"DepthTex作成失敗",L"Kinect",MB_OK);
		Clean();
		return false;
	}

	if (! m_view.Setup(hWnd)) {
		if(EngFlag) MessageBox(hWnd,L"cannot create DxOpenNI Window",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"DxOpenNI の Window 作成失敗",L"Kinect",MB_OK);
		Clean();
		return false;
	}

	return true;
}

void NISystem::Clean()
{
	m_view.Clean();
	if(m_pDepthTex){
		m_pDepthTex->Release();
		m_pDepthTex=NULL;
	}
	m_Context.Shutdown();
	for (size_t i=0; i<m_Users.size(); ++i) {
		m_Users[i]->Clean();
	}
}


//--------------------------------------------------------------------
void NISystem::OnNewUser(xn::UserGenerator& generator, XnUserID nId)
{
	// New user found
	if (m_bNeedPose){
		m_UserGenerator.GetPoseDetectionCap().StartPoseDetection(m_strPose, nId);
	}else{
		m_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}

void NISystem::OnLostUser(xn::UserGenerator& generator, XnUserID nId)
{
}

void NISystem::OnCalibrationStart(xn::SkeletonCapability& capability, XnUserID nId)
{
}

void NISystem::OnCalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess)
{
	if (bSuccess){
		NIUserData* data = NULL;
		for (size_t i=0; i<m_Users.size(); ++i) {
			if (! m_Users[i]->IsBind()) { continue; }
			if (m_Users[i]->GetXnId() != nId) { continue; }
			data = m_Users[i];
			break;
		}
		if (data == NULL) {
			for (size_t i=0; i<m_Users.size(); ++i) {
				if (m_Users[i]->IsBind()) { continue; }
				if (! m_Users[i]->Bind(nId)) { continue; }
				data = m_Users[i];
				break;
			}
		}
		if (data == NULL) { return; }
		m_UserGenerator.GetSkeletonCap().StartTracking(nId);

	}else{
		if(m_bNeedPose){
			m_UserGenerator.GetPoseDetectionCap().StartPoseDetection(m_strPose, nId);
		}else{
			m_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

void NISystem::OnPoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId)
{
	m_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	m_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}


//------------------------------------------------------------------
void NISystem::Turn(bool waitflag)
{
	DrawDepthMap(waitflag);
	UpdateSkeleton();
}

/*
XnBool					g_bDrawPixels = TRUE;
XnBool					g_bDrawBackground = FALSE;
XnBool					g_bQuit = FALSE;
int						texWidth;
int						texHeight;
XnBool					TrackingF = FALSE;
XnFloat					Colors[][3] ={{0,1,1},{0,0,1},{0,1,0},{1,1,0},{1,0,0},{1,.5,0},{.5,1,0},{0,.5,1},{.5,0,1},{1,1,.5},{1,1,1}};
*/
static XnFloat Colors[][3] ={{0,1,1},{0,0,1},{0,1,0},{1,1,0},{1,0,0},{1,.5,0},{.5,1,0},{0,.5,1},{.5,0,1},{1,1,.5},{1,1,1}};

void NISystem::DrawDepthMap(bool waitflag)
{

	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	m_DepthGenerator.GetMetaData(depthMD);
	if(waitflag) m_Context.WaitAndUpdateAll();
	else		 m_Context.WaitNoneUpdateAll();
	m_DepthGenerator.GetMetaData(depthMD);
	m_UserGenerator.GetUserPixels(0, sceneMD);

	const XnDepthPixel* pDepth = depthMD.Data();
	const XnLabel* pLabels = sceneMD.Data();
	XnUInt16 nXRes = depthMD.XRes();
	XnUInt16 nYRes = depthMD.YRes();

	D3DLOCKED_RECT LPdest;
	m_pDepthTex->LockRect(0,&LPdest,NULL, 0);
	UCHAR *pDestImage=(UCHAR*)LPdest.pBits;

	// Calculate the accumulative histogram
	ZeroMemory(m_pDepthHist,MAX_DEPTH*sizeof(float));
	UINT nValue=0;
	UINT nNumberOfPoints = 0;
	for(int nY=0;nY<nYRes;nY++){
		for(int nX=0;nX<nXRes;nX++){
			nValue = *pDepth;
			if(nValue !=0){
				m_pDepthHist[nValue]++;
				nNumberOfPoints++;
			}
			pDepth++;
		}
	}

	for(int nIndex=1;nIndex<MAX_DEPTH;nIndex++){
		m_pDepthHist[nIndex] += m_pDepthHist[nIndex-1];
	}

	if(nNumberOfPoints){
		for(int nIndex=1;nIndex<MAX_DEPTH;nIndex++){
			m_pDepthHist[nIndex] = (float)((UINT)(256 * (1.0f - (m_pDepthHist[nIndex] / nNumberOfPoints))));
		}
	}

	pDepth = depthMD.Data();
	UINT nHistValue = 0;
//	if(g_bDrawPixels){
	if(true) {
		// Prepare the texture map
		for(int nY=0;nY < nYRes;nY+=4){
			for(int nX=0;nX < nXRes;nX+=4){
				pDestImage[0] = 0;
				pDestImage[1] = 0;
				pDestImage[2] = 0;
				pDestImage[3] = 0;
//				if(g_bDrawBackground || *pLabels != 0){
				if(*pLabels != 0){
					nValue = *pDepth;
					XnLabel label = *pLabels;
					XnUInt32 nColorID = label % NCOLORS;
					if(label == 0){
						nColorID = NCOLORS;
					}

					if(nValue != 0){
						nHistValue = (UINT)(m_pDepthHist[nValue]);

						pDestImage[0] = (UINT)(nHistValue * Colors[nColorID][0]); 
						pDestImage[1] = (UINT)(nHistValue * Colors[nColorID][1]);
						pDestImage[2] = (UINT)(nHistValue * Colors[nColorID][2]);
						pDestImage[3] = 255;
					}
				}

				pDepth+=4;
				pLabels+=4;
				pDestImage+=4;
			}

			int pg = nXRes*3;
			pDepth += pg;
			pLabels += pg;
			pDestImage += (m_texWidth - nXRes)*4+pg;
		}
	}else{
		xnOSMemSet(LPdest.pBits,0,4*2*nXRes*nYRes);
	}
	m_pDepthTex->UnlockRect(0);
}

void NISystem::UpdateSkeleton()
{
	if (m_target != NULL) {
		if (! m_UserGenerator.GetSkeletonCap().IsTracking(m_target->GetXnId())) {
			m_target = NULL;
		}
	}

	if (m_target == NULL) {
		XnUserID aUsers[15];
		XnUInt16 nUsers = 15;
		m_UserGenerator.GetUsers(aUsers, nUsers);
		for (int i=0;i<nUsers;++i) {
			if(! m_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])){
				continue;
			}

			for (size_t j=0; j<m_Users.size(); ++j) {
				if (m_Users[j]->GetXnId() == aUsers[i]) {
					m_target = m_Users[j];
					break;
				}
			}
			if (m_target == NULL) {
				m_UserGenerator.GetSkeletonCap().StopTracking(aUsers[i]);
				continue;
			}
			break;
		}
	}
	if (m_target == NULL) { return; }

	m_target->Update(m_UserGenerator);
}

void NISystem::GetDepthTexture(IDirect3DTexture9** lpTex)
{
	*lpTex = m_pDepthTex;
}

void NISystem::IsTracking(bool* lpb)
{
	*lpb = (m_target != NULL && m_target->IsReady());
}

void NISystem::GetJointPosition(int num,D3DXVECTOR3* vec)
{
	if (m_target) {
		m_target->GetJointPosition(num, vec);
	}
}
