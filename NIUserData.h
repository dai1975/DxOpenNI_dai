#ifndef MMD_DXOPENNI_NIUSERDATA_H
#define MMD_DXOPENNI_NIUSERDATA_H

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnCppWrapper.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "DxOpenNI.h"
#include "tc.h"
#include "NICommonParam.h"

namespace DxOpenNI {

class NIUserData
{
public:
	NIUserData();
	virtual ~NIUserData();

	bool Setup();
	void Clean();
	bool Bind(XnUserID xnId, NICommonParam*);
	void Release();

	XnUserID GetXnId() const { return m_xnId; }
	inline bool IsBind() const { return m_bind; }
	inline bool IsReady() const { return m_ready == 0; }

public:
	void Update(xn::UserGenerator&);
	inline void GetJointPosition(JOINT_INDEX i, D3DXVECTOR3* vec);
	inline void ClearCache();

	struct JointData {
		D3DXVECTOR3 pos;
		float confidence;
	};
	struct Snap {
		JointData joint[NUM_JOINT];

		inline Snap() {
			for (int i=0; i<NUM_JOINT; ++i) {
				joint[i].pos.x = joint[i].pos.y = joint[i].pos.z = 0;
				joint[i].confidence = 0.0f;
			}
		}
		inline Snap(const Snap& r) { operator=(r); }
		inline Snap& operator=(const Snap& r) {
			for (int i=0; i<NUM_JOINT; ++i) {
				joint[i].pos = r.joint[i].pos;
				joint[i].confidence = r.joint[i].confidence;
			}
			return *this;
		}
	};

private:
	inline void ReadyData(JOINT_INDEX i);
	void CalcCache(JOINT_INDEX i);

	Snap m_cache;
	bool m_hasCache[ NICommonParam::MAX_HISTORY ];
	Snap m_history[ NICommonParam::MAX_HISTORY ];
	int m_historyLatest;

private:
	XnUserID m_xnId;
	NICommonParam* m_commonParam;

	bool m_bind;
	int m_ready;
	XnSkeletonJointPosition m_zero;
};

inline void NIUserData::ClearCache()
{
	for (int i=0; i<NUM_JOINT; ++i) { m_hasCache[i] = false; }
}

inline void NIUserData::ReadyData(JOINT_INDEX i)
{
	if (! m_hasCache[i]) {
		CalcCache(i);
		m_hasCache[i] = true;
	}
}

inline void NIUserData::GetJointPosition(JOINT_INDEX i, D3DXVECTOR3* vec)
{
	ReadyData(i);
	*vec = m_cache.joint[i].pos;
}

}

#endif

