#ifndef MMD_DXOPENNI_NIUSERDATA_H
#define MMD_DXOPENNI_NIUSERDATA_H

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnCppWrapper.h>
#include <d3d9.h>
#include <d3dx9.h>

class NIUserData
{
public:
	NIUserData();
	virtual ~NIUserData();

	bool Setup();
	void Clean();
	bool Bind(XnUserID xnId);
	void Release();

	XnUserID GetXnId() const { return m_xnId; }
	inline bool IsBind() const { return m_bind; }
	inline bool IsReady() const { return m_ready == 0; }

	void Update(xn::UserGenerator&);
	void GetJointPosition(int num,D3DXVECTOR3* vec);

public:
	void toggleTest() { m_test = !m_test; }
	bool isTest() const { return m_test; }

private:
	XnUserID m_xnId;

	bool m_bind;
	int m_ready;

	XnSkeletonJointPosition m_zero;
	static XnSkeletonJoint MMD2XN[];

	struct Data {
		D3DXVECTOR3 vec;
	};
	D3DXVECTOR3				   m_BP_Vector[15]; // 0:center 1:neck 2:head 3:shoulderL 4:elbowL 5:handL 6:shoulderR 7:elbowR 8:handR 9:legL 10:kneeL 11 ancleL 12:legR 13:kneeR 14:ancleR

	D3DXVECTOR3 m_testVec;
	bool m_test;
};

#endif

