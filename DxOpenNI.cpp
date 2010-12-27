// DxOpenNI.cpp
//  Dinamic link library of OpenNI for DirectX named DxOpenNI.dll
//
//   This program is modified from OpenNI driver.
//   OpenNI is written and distributed under the GNU Lesser General Public License,
//  so this program(dll) is redistributed under the terms of the GNU Lesser General Public License
//  as published by the Free Software Foundation, either version 3 of the License.
//   See the GNU General Public License for more details: <http://www.gnu.org/licenses/>.
// 
//   このプログラムはOpenNIドライバを使用しています。OpenNIはGNU LGPLライセンスです。
//   よって、このプログラムもLGPLライセンスに従い、ソースコードを公開することとします。
//   なお、このプログラムはダイナミックリンク(dll)として使用する場合、このdllを利用する
//  他のプログラムはLGPLライセンスに従う必要はありません。


// include files
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnCppWrapper.h>
#include <d3d9.h>
#include <d3dx9.h>

// export functions
__declspec(dllexport) bool __stdcall OpenNIInit(HWND,bool,LPDIRECT3DDEVICE9,WCHAR*);
__declspec(dllexport) void __stdcall OpenNIClean();
__declspec(dllexport) void __stdcall OpenNIDrawDepthMap(bool);
__declspec(dllexport) void __stdcall OpenNIDepthTexture(IDirect3DTexture9**);
__declspec(dllexport) void __stdcall OpenNIGetSkeltonJointPosition(int,D3DXVECTOR3*);
__declspec(dllexport) void __stdcall OpenNIIsTracking(bool*);



// include libraries
#pragma comment(lib, "openNI.lib")

// defines
#define MAX_DEPTH	10000
#define NCOLORS		10

// callbacks
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator&,XnUserID,void*);
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator&,XnUserID,void*);
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability&,XnUserID,void*);
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability&,XnUserID,XnBool,void*);
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability&,const XnChar*,XnUserID,void*);

// global variables
xn::Context				g_Context;
xn::DepthGenerator		g_DepthGenerator;
XnBool					g_bDrawPixels = TRUE;
XnBool					g_bDrawBackground = FALSE;
XnBool					g_bQuit = FALSE;
int						texWidth;
int						texHeight;
int						TrCount[15];
float					g_pDepthHist[MAX_DEPTH];
XnSkeletonJointPosition BP_Zero;
D3DXVECTOR3				BP_Vector[15]; // 0:center 1:neck 2:head 3:shoulderL 4:elbowL 5:handL 6:shoulderR 7:elbowR 8:handR 9:legL 10:kneeL 11 ancleL 12:legR 13:kneeR 14:ancleR
IDirect3DTexture9*		DepthTex = NULL;
XnBool					TrackingF = FALSE;
xn::UserGenerator		g_UserGenerator;
XnBool					g_bNeedPose = FALSE;
XnChar					g_strPose[20] = "";
XnFloat					Colors[][3] ={{0,1,1},{0,0,1},{0,1,0},{1,1,0},{1,0,0},{1,.5,0},{.5,1,0},{0,.5,1},{.5,0,1},{1,1,.5},{1,1,1}};

// DllMain
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_PROCESS_DETACH:
		OpenNIClean();
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

// CALLBACK:User_NewUser()
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	// New user found
	if (g_bNeedPose){
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}else{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}

// CALLBACK:User_LostUser()
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
}

// CALLBACK:UserCalibration_CalibrationStart()
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
}

// CALLBACK:UserCalibration_CalibrationEnd()
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie)
{
	if (bSuccess){
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}else{
		if(g_bNeedPose){
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}else{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

// CALLBACK:UserPose_PoseDetected()
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// FUNCTION:getClosestPowerOfTwo()
UINT getClosestPowerOfTwo(UINT n)
{
	unsigned int m = 2;
	while(m < n) m<<=1;

	return m;
}

// FUNCTION:PosCalc()
void PosCalc(XnUserID player,XnSkeletonJoint ejoint,D3DXVECTOR3* point)
{
	XnSkeletonJointPosition jointx;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,ejoint,jointx);
	if(jointx.fConfidence<0.5f){
		point->y=-999.0f;
	}else{
		point->x=(jointx.position.X-BP_Zero.position.X);
		point->y=(jointx.position.Y-BP_Zero.position.Y);
		point->z=(jointx.position.Z-BP_Zero.position.Z);
	}
}

// FUNCTION:printError()
void printError(HWND hWnd,const char *name, XnStatus nRetVal)
{
	char moji[256];
	sprintf_s(moji,sizeof(moji),"%s failed: %s\n", name, xnGetStatusString(nRetVal));
	MessageBoxA(hWnd,moji,"error",MB_OK);
}

// EXPORT FUNCTION:Clean()
__declspec(dllexport) void __stdcall OpenNIClean()
{
	if(DepthTex){
		DepthTex->Release();
		DepthTex=NULL;
	}
	g_Context.Shutdown();
	TrackingF=false;
}

// EXPORT FUNCTION:Init()
__declspec(dllexport) bool __stdcall OpenNIInit(HWND hWnd,bool EngFlag,LPDIRECT3DDEVICE9 lpDevice,WCHAR* f_path)
{
	TrackingF=false;
	for(int i=0;i<15;i++) TrCount[i]=0;

	SetCurrentDirectoryW(f_path);

	FILE *fp;
	if((fp=fopen("Data\\SamplesConfig.xml","r"))!=NULL){
		fclose( fp );
	}else{
		if(EngFlag) MessageBox(hWnd,L"SamplesConfig.xml cannot find in Data folder.\n\nPlease download DxOpenNI and get SamplesConfig.xml from in it\nand put it into \"Data\" folder of MMD.",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"SamplesConfig.xmlがDataフォルダ内にありません。\n\nDxOpenNIをダウンロードしてその中からSamplesConfig.xmlをMMDのDataフォルダに入れて下さい。",L"Kinect",MB_OK);
		return false;
	}
	XnStatus nRetVal = g_Context.InitFromXmlFile("Data\\SamplesConfig.xml");
	if(nRetVal != XN_STATUS_OK){
		printError(hWnd,"InitFromXmlFile", nRetVal);
		if(EngFlag) MessageBox(hWnd,L"Cannot find Kinect sensor",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectが接続されていません",L"Kinect",MB_OK);
		return false;
	}

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	if(nRetVal != XN_STATUS_OK){
		printError(hWnd,"FindExistingNode", nRetVal);
		if(EngFlag) MessageBox(hWnd,L"Cannot find Kinect Depth generator",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectの深度センサーを認識できません",L"Kinect",MB_OK);
		OpenNIClean();
		return false;
	}

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if(nRetVal != XN_STATUS_OK){
		nRetVal = g_UserGenerator.Create(g_Context);
		if(nRetVal != XN_STATUS_OK){
			printError(hWnd,"g_UserGenerator.Create", nRetVal);
			if(EngFlag) MessageBox(hWnd,L"Cannot find Kinect User generator",L"Kinect",MB_OK);
			else		MessageBox(hWnd,L"Kinectのユーザーセンサーを認識できません",L"Kinect",MB_OK);
			OpenNIClean();
			return false;
		}
	}

	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;
	if(!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)){
		if(EngFlag) MessageBox(hWnd,L"Supplied user generator doesn't support skeleton",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectのユーザーセンサーがボーン構造をサポートしていません",L"Kinect",MB_OK);
		OpenNIClean();
		return false;
	}
	g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, NULL, hCalibrationCallbacks);

	if(g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration()){
		g_bNeedPose = TRUE;
		if(!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)){
			if(EngFlag) MessageBox(hWnd,L"Pose required, but not supported",L"Kinect",MB_OK);
			else		MessageBox(hWnd,L"Kinectがキャリブレーションポーズをサポートしていません",L"Kinect",MB_OK);
			OpenNIClean();
			return false;
		}
		g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, NULL, hPoseCallbacks);
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}
	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	nRetVal = g_Context.StartGeneratingAll();
	if(nRetVal != XN_STATUS_OK){
		printError(hWnd,"StartGeneratingAll", nRetVal);
		if(EngFlag) MessageBox(hWnd,L"Cannot start Kinect generating",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"Kinectセンサーを始動できません",L"Kinect",MB_OK);
		OpenNIClean();
		return false;
	}

	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	g_DepthGenerator.GetMetaData(depthMD);
	g_Context.WaitAndUpdateAll();
	g_DepthGenerator.GetMetaData(depthMD);
	g_UserGenerator.GetUserPixels(0, sceneMD);

	int x=depthMD.XRes();
	int y=depthMD.YRes();
	texWidth =  getClosestPowerOfTwo(x/4);
	texHeight = getClosestPowerOfTwo(y/4);

	if(FAILED(lpDevice->CreateTexture(texWidth,texHeight,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&DepthTex,NULL))){
		if(EngFlag) MessageBox(hWnd,L"cannot make DepthTex",L"Kinect",MB_OK);
		else		MessageBox(hWnd,L"DepthTex作成失敗",L"Kinect",MB_OK);
		OpenNIClean();
		return false;
	}

	return true;
}

// EXPORT FUNCTION:DrawDepthMap()
__declspec(dllexport) void __stdcall OpenNIDrawDepthMap(bool waitflag)
{
	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	g_DepthGenerator.GetMetaData(depthMD);
	if(waitflag) g_Context.WaitAndUpdateAll();
	else		 g_Context.WaitNoneUpdateAll();
	g_DepthGenerator.GetMetaData(depthMD);
	g_UserGenerator.GetUserPixels(0, sceneMD);

	const XnDepthPixel* pDepth = depthMD.Data();
	const XnLabel* pLabels = sceneMD.Data();
	XnUInt16 g_nXRes = depthMD.XRes();
	XnUInt16 g_nYRes = depthMD.YRes();

	D3DLOCKED_RECT LPdest;
	DepthTex->LockRect(0,&LPdest,NULL, 0);
	UCHAR *pDestImage=(UCHAR*)LPdest.pBits;

	// Calculate the accumulative histogram
	ZeroMemory(g_pDepthHist,MAX_DEPTH*sizeof(float));
	UINT nValue=0;
	UINT nNumberOfPoints = 0;
	for(int nY=0;nY<g_nYRes;nY++){
		for(int nX=0;nX<g_nXRes;nX++){
			nValue = *pDepth;
			if(nValue !=0){
				g_pDepthHist[nValue]++;
				nNumberOfPoints++;
			}
			pDepth++;
		}
	}

	for(int nIndex=1;nIndex<MAX_DEPTH;nIndex++){
		g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
	}

	if(nNumberOfPoints){
		for(int nIndex=1;nIndex<MAX_DEPTH;nIndex++){
			g_pDepthHist[nIndex] = (float)((UINT)(256 * (1.0f - (g_pDepthHist[nIndex] / nNumberOfPoints))));
		}
	}

	pDepth = depthMD.Data();
	UINT nHistValue = 0;
	if(g_bDrawPixels){
		// Prepare the texture map
		for(int nY=0;nY < g_nYRes;nY+=4){
			for(int nX=0;nX < g_nXRes;nX+=4){
				pDestImage[0] = 0;
				pDestImage[1] = 0;
				pDestImage[2] = 0;
				pDestImage[3] = 0;
				if(g_bDrawBackground || *pLabels != 0){
					nValue = *pDepth;
					XnLabel label = *pLabels;
					XnUInt32 nColorID = label % NCOLORS;
					if(label == 0){
						nColorID = NCOLORS;
					}

					if(nValue != 0){
						nHistValue = (UINT)(g_pDepthHist[nValue]);

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

			int pg = g_nXRes*3;
			pDepth += pg;
			pLabels += pg;
			pDestImage += (texWidth - g_nXRes)*4+pg;
		}
	}else{
		xnOSMemSet(LPdest.pBits,0,4*2*g_nXRes*g_nYRes);
	}
	DepthTex->UnlockRect(0);

	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for(int i=0;i<nUsers;++i){
		if(g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])){
			if(TrCount[i]<4){
				TrCount[i]++;
				if(TrCount[i]==4){
					TrackingF=true;
					g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_TORSO, BP_Zero);
				}
			}
			PosCalc(aUsers[i],XN_SKEL_TORSO,&BP_Vector[0]);
			PosCalc(aUsers[i],XN_SKEL_NECK,&BP_Vector[1]);
			PosCalc(aUsers[i],XN_SKEL_HEAD,&BP_Vector[2]);
			PosCalc(aUsers[i],XN_SKEL_LEFT_SHOULDER,&BP_Vector[3]);
			PosCalc(aUsers[i],XN_SKEL_LEFT_ELBOW,&BP_Vector[4]);
			PosCalc(aUsers[i],XN_SKEL_LEFT_HAND,&BP_Vector[5]);
			PosCalc(aUsers[i],XN_SKEL_RIGHT_SHOULDER,&BP_Vector[6]);
			PosCalc(aUsers[i],XN_SKEL_RIGHT_ELBOW,&BP_Vector[7]);
			PosCalc(aUsers[i],XN_SKEL_RIGHT_HAND,&BP_Vector[8]);
			PosCalc(aUsers[i],XN_SKEL_LEFT_HIP,&BP_Vector[9]);
			PosCalc(aUsers[i],XN_SKEL_LEFT_KNEE,&BP_Vector[10]);
			PosCalc(aUsers[i],XN_SKEL_LEFT_FOOT,&BP_Vector[11]);
			PosCalc(aUsers[i],XN_SKEL_RIGHT_HIP,&BP_Vector[12]);
			PosCalc(aUsers[i],XN_SKEL_RIGHT_KNEE,&BP_Vector[13]);
			PosCalc(aUsers[i],XN_SKEL_RIGHT_FOOT,&BP_Vector[14]);
			break;
		}else{
			TrCount[i]=0;
		}
	}
}

// DepthTexture()
__declspec(dllexport) void __stdcall OpenNIDepthTexture(IDirect3DTexture9** lpTex)
{
	*lpTex = DepthTex;
}

// GetSkeltonJointPosition()
__declspec(dllexport) void __stdcall OpenNIGetSkeltonJointPosition(int num,D3DXVECTOR3* vec)
{
	*vec = BP_Vector[num];
}

// IsTracking()
__declspec(dllexport) void __stdcall OpenNIIsTracking(bool* lpb)
{
	if(TrackingF) *lpb = true;
	else		  *lpb = false;
}