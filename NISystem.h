#ifndef MMD_DXOPENNI_NISYSTEM_H
#define MMD_DXOPENNI_NISYSTEM_H

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnCppWrapper.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "View.h"
#include "NIUserData.h"
#include <vector>

class NISystem
{
public:
	NISystem();
	virtual ~NISystem();

public: //DLL init/finish
	bool OnAttach(HINSTANCE);
	bool OnDetach(HINSTANCE);

public: //callbacks from OpenNI
	void OnNewUser(xn::UserGenerator& generator, XnUserID nId);
	void OnLostUser(xn::UserGenerator& generator, XnUserID nId);
	void OnCalibrationStart(xn::SkeletonCapability& capability, XnUserID nId);
	void OnCalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess);
	void OnPoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId);


public: //call by application
	bool Setup(HWND hWnd, bool EngFlag, LPDIRECT3DDEVICE9 lpDevice, const char* file);
	void Clean();

	void Turn(bool waitflag);
	void GetDepthTexture(IDirect3DTexture9** lpTex);
	void IsTracking(bool* lpb);
	void GetJointPosition(int num,D3DXVECTOR3* vec);

public: //GUI controll
	inline void toggleTest() { if (m_target) { m_target->toggleTest(); } }
	inline bool isTest() const { return (m_target && m_target->isTest()); }

private:
	void DrawDepthMap(bool waitflag);
	void UpdateSkeleton();

private:
	//state
	std::vector< NIUserData* > m_Users;
	NIUserData* m_target;

	// reference
	View m_view;
	IDirect3DTexture9* m_pDepthTex;
	xn::Context        m_Context;
	xn::DepthGenerator m_DepthGenerator;
	xn::UserGenerator  m_UserGenerator;

	// config
	bool   m_bNeedPose;
	XnChar m_strPose[20];

	// allocated memory
	enum {
		MAX_DEPTH = 10000,
		NCOLORS = 10,
	};
	int m_texWidth, m_texHeight;
	float m_pDepthHist[MAX_DEPTH];
};

#endif
