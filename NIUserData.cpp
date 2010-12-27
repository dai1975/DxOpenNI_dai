#include "NIUserData.h"

XnSkeletonJoint NIUserData::MMD2XN[] = {
		XN_SKEL_TORSO,
		XN_SKEL_NECK,
		XN_SKEL_HEAD,
		XN_SKEL_LEFT_SHOULDER,
		XN_SKEL_LEFT_ELBOW,
		XN_SKEL_LEFT_HAND,
		XN_SKEL_RIGHT_SHOULDER,
		XN_SKEL_RIGHT_ELBOW,
		XN_SKEL_RIGHT_HAND,
		XN_SKEL_LEFT_HIP,
		XN_SKEL_LEFT_KNEE,
		XN_SKEL_LEFT_FOOT,
		XN_SKEL_RIGHT_HIP,
		XN_SKEL_RIGHT_KNEE,
		XN_SKEL_RIGHT_FOOT,
	};

NIUserData::NIUserData()
{ }

NIUserData::~NIUserData()
{ }

bool NIUserData::Setup()
{
	m_bind = false;
	m_ready = 4;
	m_test = false;
	return true;
}

void NIUserData::Clean()
{
	m_bind = false;
	m_ready = 4;
}

bool NIUserData::Bind(XnUserID xnId)
{
	if (m_bind) { return false; }

	m_xnId = xnId;
	m_bind = true;
	m_ready = 4;
	m_test = false;
	m_testVec.x = 0;
	m_testVec.y = 0;
	m_testVec.z = 0;
	return true;
}

void NIUserData::Release()
{
	m_bind = false;
}

namespace {

inline XnStatus _GetJointPos
(xn::UserGenerator& gen,
 XnUserID id,
 XnSkeletonJoint joint,
 XnSkeletonJointPosition& ret
 )
{
	return gen.GetSkeletonCap().GetSkeletonJointPosition(id, joint, ret);
}

}

void NIUserData::Update(xn::UserGenerator& gen)
{
	if (0 < m_ready) {
		if (m_ready == 1) {
			//gen.GetSkeletonCap().GetSkeletonJointPosition(m_xnId, XN_SKEL_TORSO, m_zero);
			// 1.1
			XnSkeletonJointPosition sjp1,sjp2;
			_GetJointPos(gen, m_xnId, XN_SKEL_LEFT_HIP, sjp1);
			_GetJointPos(gen, m_xnId, XN_SKEL_RIGHT_HIP, sjp2);
			m_zero.position.X = (sjp1.position.X+sjp2.position.X)/2.0f;
			m_zero.position.Y = (sjp1.position.Y+sjp2.position.Y)/2.0f;
			m_zero.position.Z = (sjp1.position.Z+sjp2.position.Z)/2.0f;
		}
		--m_ready;
	}

	for (int i=1; i<15; ++i) {
		XnSkeletonJoint interest = MMD2XN[i];
		XnSkeletonJointPosition jointx;
		_GetJointPos(gen, m_xnId, interest, jointx);
		if(jointx.fConfidence<0.5f){
			m_BP_Vector[i].y=-999.0f;
		}else{
			m_BP_Vector[i].x = (jointx.position.X - m_zero.position.X);
			m_BP_Vector[i].y = (jointx.position.Y - m_zero.position.Y);
			m_BP_Vector[i].z = (jointx.position.Z - m_zero.position.Z);
		}
	}
	m_BP_Vector[0] = (m_BP_Vector[9] + m_BP_Vector[12])/2.0f;

	if (m_test) {
		m_testVec.x += 10;
	}
}

void NIUserData::GetJointPosition(int num,D3DXVECTOR3* vec)
{
	vec->x = m_BP_Vector[num].x + m_testVec.x;
	vec->y = m_BP_Vector[num].y + m_testVec.y;
	vec->z = m_BP_Vector[num].z + m_testVec.z;
}
