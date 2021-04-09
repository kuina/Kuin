#include "draw_obj.h"

const U8* GetBoxKnobjBin(size_t* size);
const U8* GetSphereKnobjBin(size_t* size);
const U8* GetPlaneKnobjBin(size_t* size);

static void ObjDrawImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
static void ObjDrawToonImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow);
static void ObjDrawFlatImpl(SClass* me_, S64 element, double frame, SClass* diffuse);
static void WriteFastPsConstBuf(SObjFastPsConstBuf* ps_const_buf);
static SClass* MakeObjImpl(SClass* me_, size_t size, const void* binary);

EXPORT_CPP void _objDraw(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal)
{
	ObjDrawImpl(me_, element, frame, diffuse, specular, normal, nullptr);
}

EXPORT_CPP void _objDrawFlat(SClass* me_, S64 element, double frame, SClass* diffuse)
{
	ObjDrawFlatImpl(me_, element, frame, diffuse);
}

EXPORT_CPP void _objDrawOutline(SClass* me_, S64 element, double frame, double width, S64 color)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, 0xe9170006);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), 0xe9170006);
				if (element2->JointNum < 0 || JointMax < element2->JointNum)
					THROW(0xe9170008);
				Bool joint = element2->JointNum != 0;

				SObjOutlineVsConstBuf vs_const_buf;
				SObjOutlinePsConstBuf ps_const_buf;
				vs_const_buf.CommonParam = ObjVsConstBuf.CommonParam;
				vs_const_buf.OutlineParam[0] = static_cast<float>(width);
				{
					double r, g, b, a;
					ColorToArgb(&a, &r, &g, &b, color);
					ps_const_buf.OutlineColor[0] = static_cast<float>(r);
					ps_const_buf.OutlineColor[1] = static_cast<float>(g);
					ps_const_buf.OutlineColor[2] = static_cast<float>(b);
					ps_const_buf.OutlineColor[3] = static_cast<float>(a);
				}

				if (joint && frame >= 0.0f)
				{
					SetJointMat(element2, frame, vs_const_buf.Joint);
					ConstBuf(ShaderBufs[ShaderBuf_ObjOutlineJointVs], &vs_const_buf);
				}
				else
					ConstBuf(ShaderBufs[ShaderBuf_ObjOutlineVs], &vs_const_buf);
				Device->GSSetShader(nullptr);
				Device->RSSetState(RasterizerStates[RasterizerState_Inverted]);
				ConstBuf(ShaderBufs[ShaderBuf_ObjOutlinePs], &ps_const_buf);

				VertexBuf(element2->VertexBuf);
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
				Device->RSSetState(RasterizerStates[RasterizerState_Normal]);
			}
			break;
	}
}

EXPORT_CPP void _objDrawToon(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal)
{
	ObjDrawToonImpl(me_, element, frame, diffuse, specular, normal, nullptr);
}

EXPORT_CPP void _objDrawToonWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	ObjDrawToonImpl(me_, element, frame, diffuse, specular, normal, shadow);
}

EXPORT_CPP void _objDrawWithShadow(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	ObjDrawImpl(me_, element, frame, diffuse, specular, normal, shadow);
}

EXPORT_CPP void _objFin(SClass* me_)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	for (int i = 0; i < me2->ElementNum; i++)
	{
		switch (me2->ElementKinds[i])
		{
			case 0: // Polygon.
				{
					SObj::SPolygon* element = static_cast<SObj::SPolygon*>(me2->Elements[i]);
					if (element->Joints != nullptr)
						FreeMem(element->Joints);
					if (element->VertexBuf != nullptr)
						FinVertexBuf(element->VertexBuf);
					FreeMem(element);
				}
				break;
			default:
				ASSERT(False);
				break;
		}
	}
	me2->ElementNum = 0;
	if (me2->Elements != nullptr)
	{
		FreeMem(me2->Elements);
		me2->Elements = nullptr;
	}
	if (me2->ElementKinds != nullptr)
	{
		FreeMem(me2->ElementKinds);
		me2->ElementKinds = nullptr;
	}
}

EXPORT_CPP void _objLook(SClass* me_, double x, double y, double z, double atX, double atY, double atZ, double upX, double upY, double upZ, Bool fixUp)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	double at[3] = { atX - x, atY - y, atZ - z }, up[3] = { upX, upY, upZ }, right[3];
	if (Normalize(at) == 0.0)
		return;
	if (Normalize(up) == 0.0)
		return;
	Cross(right, up, at);
	if (Normalize(right) == 0.0)
		return;
	if (fixUp)
		Cross(up, at, right);
	else
		Cross(at, right, up);
	me2->Mat[0][0] = static_cast<float>(right[0]);
	me2->Mat[0][1] = static_cast<float>(right[1]);
	me2->Mat[0][2] = static_cast<float>(right[2]);
	me2->Mat[0][3] = 0.0f;
	me2->Mat[1][0] = static_cast<float>(up[0]);
	me2->Mat[1][1] = static_cast<float>(up[1]);
	me2->Mat[1][2] = static_cast<float>(up[2]);
	me2->Mat[1][3] = 0.0f;
	me2->Mat[2][0] = static_cast<float>(at[0]);
	me2->Mat[2][1] = static_cast<float>(at[1]);
	me2->Mat[2][2] = static_cast<float>(at[2]);
	me2->Mat[2][3] = 0.0f;
	me2->Mat[3][0] = static_cast<float>(x);
	me2->Mat[3][1] = static_cast<float>(y);
	me2->Mat[3][2] = static_cast<float>(z);
	me2->Mat[3][3] = 1.0f;
	me2->NormMat[0][0] = static_cast<float>(right[0]);
	me2->NormMat[0][1] = static_cast<float>(right[1]);
	me2->NormMat[0][2] = static_cast<float>(right[2]);
	me2->NormMat[0][3] = 0.0f;
	me2->NormMat[1][0] = static_cast<float>(up[0]);
	me2->NormMat[1][1] = static_cast<float>(up[1]);
	me2->NormMat[1][2] = static_cast<float>(up[2]);
	me2->NormMat[1][3] = 0.0f;
	me2->NormMat[2][0] = static_cast<float>(at[0]);
	me2->NormMat[2][1] = static_cast<float>(at[1]);
	me2->NormMat[2][2] = static_cast<float>(at[2]);
	me2->NormMat[2][3] = 0.0f;
	me2->NormMat[3][0] = 0.0f;
	me2->NormMat[3][1] = 0.0f;
	me2->NormMat[3][2] = 0.0f;
	me2->NormMat[3][3] = 1.0f;
}

EXPORT_CPP void _objLookCamera(SClass* me_, double x, double y, double z, double upX, double upY, double upZ, Bool fixUp)
{
	_objLook(me_, x, y, z, x + static_cast<double>(ObjVsConstBuf.CommonParam.Eye[0]), y + static_cast<double>(ObjVsConstBuf.CommonParam.Eye[1]), z + static_cast<double>(ObjVsConstBuf.CommonParam.Eye[2]), upX, upY, upZ, fixUp);
}

EXPORT_CPP void _objMat(SClass* me_, const U8* mat, const U8* normMat)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(*(S64*)(mat + 0x08) != 16 || *(S64*)(normMat + 0x08) != 16, 0xe9170006);
	{
		const double* ptr = reinterpret_cast<const double*>(mat + 0x10);
		me2->Mat[0][0] = static_cast<float>(ptr[0]);
		me2->Mat[0][1] = static_cast<float>(ptr[1]);
		me2->Mat[0][2] = static_cast<float>(ptr[2]);
		me2->Mat[0][3] = static_cast<float>(ptr[3]);
		me2->Mat[1][0] = static_cast<float>(ptr[4]);
		me2->Mat[1][1] = static_cast<float>(ptr[5]);
		me2->Mat[1][2] = static_cast<float>(ptr[6]);
		me2->Mat[1][3] = static_cast<float>(ptr[7]);
		me2->Mat[2][0] = static_cast<float>(ptr[8]);
		me2->Mat[2][1] = static_cast<float>(ptr[9]);
		me2->Mat[2][2] = static_cast<float>(ptr[10]);
		me2->Mat[2][3] = static_cast<float>(ptr[11]);
		me2->Mat[3][0] = static_cast<float>(ptr[12]);
		me2->Mat[3][1] = static_cast<float>(ptr[13]);
		me2->Mat[3][2] = static_cast<float>(ptr[14]);
		me2->Mat[3][3] = static_cast<float>(ptr[15]);
	}
	{
		const double* ptr = reinterpret_cast<const double*>(normMat + 0x10);
		me2->NormMat[0][0] = static_cast<float>(ptr[0]);
		me2->NormMat[0][1] = static_cast<float>(ptr[1]);
		me2->NormMat[0][2] = static_cast<float>(ptr[2]);
		me2->NormMat[0][3] = static_cast<float>(ptr[3]);
		me2->NormMat[1][0] = static_cast<float>(ptr[4]);
		me2->NormMat[1][1] = static_cast<float>(ptr[5]);
		me2->NormMat[1][2] = static_cast<float>(ptr[6]);
		me2->NormMat[1][3] = static_cast<float>(ptr[7]);
		me2->NormMat[2][0] = static_cast<float>(ptr[8]);
		me2->NormMat[2][1] = static_cast<float>(ptr[9]);
		me2->NormMat[2][2] = static_cast<float>(ptr[10]);
		me2->NormMat[2][3] = static_cast<float>(ptr[11]);
		me2->NormMat[3][0] = static_cast<float>(ptr[12]);
		me2->NormMat[3][1] = static_cast<float>(ptr[13]);
		me2->NormMat[3][2] = static_cast<float>(ptr[14]);
		me2->NormMat[3][3] = static_cast<float>(ptr[15]);
	}
}

EXPORT_CPP void _objPos(SClass* me_, double scaleX, double scaleY, double scaleZ, double rotX, double rotY, double rotZ, double transX, double transY, double transZ)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	double cos_x = cos(rotX);
	double sin_x = sin(rotX);
	double cos_y = cos(rotY);
	double sin_y = sin(rotY);
	double cos_z = cos(rotZ);
	double sin_z = sin(rotZ);
	me2->Mat[0][0] = static_cast<float>(scaleX * (cos_y * cos_z));
	me2->Mat[0][1] = static_cast<float>(scaleX * (cos_y * sin_z));
	me2->Mat[0][2] = static_cast<float>(scaleX * (-sin_y));
	me2->Mat[0][3] = 0.0f;
	me2->Mat[1][0] = static_cast<float>(scaleY * (sin_x * sin_y * cos_z - cos_x * sin_z));
	me2->Mat[1][1] = static_cast<float>(scaleY * (sin_x * sin_y * sin_z + cos_x * cos_z));
	me2->Mat[1][2] = static_cast<float>(scaleY * (sin_x * cos_y));
	me2->Mat[1][3] = 0.0f;
	me2->Mat[2][0] = static_cast<float>(scaleZ * (cos_x * sin_y * cos_z + sin_x * sin_z));
	me2->Mat[2][1] = static_cast<float>(scaleZ * (cos_x * sin_y * sin_z - sin_x * cos_z));
	me2->Mat[2][2] = static_cast<float>(scaleZ * (cos_x * cos_y));
	me2->Mat[2][3] = 0.0f;
	me2->Mat[3][0] = static_cast<float>(transX);
	me2->Mat[3][1] = static_cast<float>(transY);
	me2->Mat[3][2] = static_cast<float>(transZ);
	me2->Mat[3][3] = 1.0f;
	scaleX = 1.0 / scaleX;
	scaleY = 1.0 / scaleY;
	scaleZ = 1.0 / scaleZ;
	me2->NormMat[0][0] = static_cast<float>(scaleX * (cos_y * cos_z));
	me2->NormMat[0][1] = static_cast<float>(scaleX * (cos_y * sin_z));
	me2->NormMat[0][2] = static_cast<float>(scaleX * (-sin_y));
	me2->NormMat[0][3] = 0.0f;
	me2->NormMat[1][0] = static_cast<float>(scaleY * (sin_x * sin_y * cos_z - cos_x * sin_z));
	me2->NormMat[1][1] = static_cast<float>(scaleY * (sin_x * sin_y * sin_z + cos_x * cos_z));
	me2->NormMat[1][2] = static_cast<float>(scaleY * (sin_x * cos_y));
	me2->NormMat[1][3] = 0.0f;
	me2->NormMat[2][0] = static_cast<float>(scaleZ * (cos_x * sin_y * cos_z + sin_x * sin_z));
	me2->NormMat[2][1] = static_cast<float>(scaleZ * (cos_x * sin_y * sin_z - sin_x * cos_z));
	me2->NormMat[2][2] = static_cast<float>(scaleZ * (cos_x * cos_y));
	me2->NormMat[2][3] = 0.0f;
	me2->NormMat[3][0] = 0.0f;
	me2->NormMat[3][1] = 0.0f;
	me2->NormMat[3][2] = 0.0f;
	me2->NormMat[3][3] = 1.0f;
}

EXPORT_CPP SClass* _makeBox(SClass* me_)
{
	size_t size;
	const U8* binary = GetBoxKnobjBin(&size);
	return MakeObjImpl(me_, size, binary);
}

EXPORT_CPP SClass* _makeObj(SClass* me_, const U8* data)
{
	const U8* buf = data + 0x10;
	size_t size = static_cast<size_t>(*reinterpret_cast<const S64*>(data + 0x08));
	SClass* obj = MakeObjImpl(me_, size, buf);
	return obj;
}

EXPORT_CPP SClass* _makePlane(SClass* me_)
{
	size_t size;
	const U8* binary = GetPlaneKnobjBin(&size);
	return MakeObjImpl(me_, size, binary);
}

EXPORT_CPP SClass* _makeSphere(SClass* me_)
{
	size_t size;
	const U8* binary = GetSphereKnobjBin(&size);
	return MakeObjImpl(me_, size, binary);
}

static void ObjDrawImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, 0xe9170006);
	Device->PSSetSamplers(1, 1, &Sampler[2]);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), 0xe9170006);
				if (element2->JointNum < 0 || JointMax < element2->JointNum)
					THROW(0xe9170008);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.CommonParam.World, me2->Mat, sizeof(float[4][4]));
				memcpy(ObjVsConstBuf.CommonParam.NormWorld, me2->NormMat, sizeof(float[4][4]));
				if ((me2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjFastJointVs] : ShaderBufs[ShaderBuf_ObjFastJointSmVs], &ObjVsConstBuf);
					}
					else
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjFastVs] : ShaderBufs[ShaderBuf_ObjFastSmVs], &ObjVsConstBuf);
					Device->GSSetShader(nullptr);

					SObjFastPsConstBuf ps_const_buf;
					WriteFastPsConstBuf(&ps_const_buf);
					ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjFastPs] : ShaderBufs[ShaderBuf_ObjFastSmPs], &ps_const_buf);
					VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[3];
					views[0] = diffuse == nullptr ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == nullptr ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					if (shadow == nullptr)
						Device->PSSetShaderResources(0, 2, views);
					else
					{
						views[2] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 3, views);
					}
				}
				else
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjJointVs] : ShaderBufs[ShaderBuf_ObjJointSmVs], &ObjVsConstBuf);
					}
					else
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjVs] : ShaderBufs[ShaderBuf_ObjSmVs], &ObjVsConstBuf);
					Device->GSSetShader(nullptr);
					ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjPs] : ShaderBufs[ShaderBuf_ObjSmPs], &ObjPsConstBuf);
					VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[4];
					views[0] = diffuse == nullptr ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == nullptr ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					views[2] = normal == nullptr ? ViewEven[2] : reinterpret_cast<STex*>(normal)->View;
					if (shadow == nullptr)
						Device->PSSetShaderResources(0, 3, views);
					else
					{
						views[3] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 4, views);
					}
				}
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

static void ObjDrawToonImpl(SClass* me_, S64 element, double frame, SClass* diffuse, SClass* specular, SClass* normal, SClass* shadow)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, 0xe9170006);
	Device->PSSetSamplers(1, 1, &Sampler[2]);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), 0xe9170006);
				if (element2->JointNum < 0 || JointMax < element2->JointNum)
					THROW(0xe9170008);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.CommonParam.World, me2->Mat, sizeof(float[4][4]));
				memcpy(ObjVsConstBuf.CommonParam.NormWorld, me2->NormMat, sizeof(float[4][4]));
				if ((me2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjFastJointVs] : ShaderBufs[ShaderBuf_ObjFastJointSmVs], &ObjVsConstBuf);
					}
					else
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjFastVs] : ShaderBufs[ShaderBuf_ObjFastSmVs], &ObjVsConstBuf);
					Device->GSSetShader(nullptr);

					SObjFastPsConstBuf ps_const_buf;
					WriteFastPsConstBuf(&ps_const_buf);
					ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjToonFastPs] : ShaderBufs[ShaderBuf_ObjToonFastSmPs], &ps_const_buf);
					VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[4];
					views[0] = diffuse == nullptr ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == nullptr ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					views[2] = ViewToonRamp;
					if (shadow == nullptr)
						Device->PSSetShaderResources(0, 3, views);
					else
					{
						views[3] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 4, views);
					}
				}
				else
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjJointVs] : ShaderBufs[ShaderBuf_ObjJointSmVs], &ObjVsConstBuf);
					}
					else
						ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjVs] : ShaderBufs[ShaderBuf_ObjSmVs], &ObjVsConstBuf);
					Device->GSSetShader(nullptr);
					ConstBuf(shadow == nullptr ? ShaderBufs[ShaderBuf_ObjToonPs] : ShaderBufs[ShaderBuf_ObjToonSmPs], &ObjPsConstBuf);
					VertexBuf(element2->VertexBuf);
					ID3D10ShaderResourceView* views[5];
					views[0] = diffuse == nullptr ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
					views[1] = specular == nullptr ? ViewEven[1] : reinterpret_cast<STex*>(specular)->View;
					views[2] = normal == nullptr ? ViewEven[2] : reinterpret_cast<STex*>(normal)->View;
					views[3] = ViewToonRamp;
					if (shadow == nullptr)
						Device->PSSetShaderResources(0, 4, views);
					else
					{
						views[4] = reinterpret_cast<SShadow*>(shadow)->DepthResView;
						Device->PSSetShaderResources(0, 5, views);
					}
				}
				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

static void ObjDrawFlatImpl(SClass* me_, S64 element, double frame, SClass* diffuse)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	THROWDBG(element < 0 || static_cast<S64>(me2->ElementNum) <= element, 0xe9170006);
	switch (me2->ElementKinds[element])
	{
		case 0: // Polygon.
			{
				SObj::SPolygon* element2 = static_cast<SObj::SPolygon*>(me2->Elements[element]);
				THROWDBG(frame < static_cast<double>(element2->Begin) || element2->End < static_cast<int>(frame), 0xe9170006);
				if (element2->JointNum < 0 || JointMax < element2->JointNum)
					THROW(0xe9170008);
				Bool joint = element2->JointNum != 0;

				memcpy(ObjVsConstBuf.CommonParam.World, me2->Mat, sizeof(float[4][4]));
				if ((me2->Format & Format_HasTangent) == 0)
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						ConstBuf(ShaderBufs[ShaderBuf_ObjFlatFastJointVs], &ObjVsConstBuf);
					}
					else
						ConstBuf(ShaderBufs[ShaderBuf_ObjFlatFastVs], &ObjVsConstBuf);
				}
				else
				{
					if (joint && frame >= 0.0f)
					{
						SetJointMat(element2, frame, ObjVsConstBuf.Joint);
						ConstBuf(ShaderBufs[ShaderBuf_ObjFlatJointVs], &ObjVsConstBuf);
					}
					else
						ConstBuf(ShaderBufs[ShaderBuf_ObjFlatVs], &ObjVsConstBuf);
				}
				Device->GSSetShader(nullptr);
				ConstBuf(ShaderBufs[ShaderBuf_ObjFlatPs], nullptr);
				VertexBuf(element2->VertexBuf);
				ID3D10ShaderResourceView* views[1];
				views[0] = diffuse == nullptr ? ViewEven[0] : reinterpret_cast<STex*>(diffuse)->View;
				Device->PSSetShaderResources(0, 1, views);

				Device->DrawIndexed(static_cast<UINT>(element2->VertexNum), 0, 0);
			}
			break;
	}
}

static void WriteFastPsConstBuf(SObjFastPsConstBuf* ps_const_buf)
{
	ps_const_buf->CommonParam = ObjPsConstBuf.CommonParam;
	double eye[3] =
	{
		static_cast<double>(ObjVsConstBuf.CommonParam.Eye[0]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Eye[1]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Eye[2])
	};
	Normalize(eye);
	double dir[3] =
	{
		static_cast<double>(ObjVsConstBuf.CommonParam.Dir[0]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Dir[1]),
		static_cast<double>(ObjVsConstBuf.CommonParam.Dir[2])
	};
	Normalize(dir);
	double half[3] =
	{
		static_cast<double>(dir[0] + eye[0]),
		static_cast<double>(dir[1] + eye[1]),
		static_cast<double>(dir[2] + eye[2])
	};
	Normalize(half);
	ps_const_buf->Eye[0] = static_cast<float>(eye[0]);
	ps_const_buf->Eye[1] = static_cast<float>(eye[1]);
	ps_const_buf->Eye[2] = static_cast<float>(eye[2]);
	ps_const_buf->Eye[3] = 0.0f;
	ps_const_buf->Dir[0] = static_cast<float>(dir[0]);
	ps_const_buf->Dir[1] = static_cast<float>(dir[1]);
	ps_const_buf->Dir[2] = static_cast<float>(dir[2]);
	ps_const_buf->Dir[3] = 0.0f;
	ps_const_buf->Half[0] = static_cast<float>(half[0]);
	ps_const_buf->Half[1] = static_cast<float>(half[1]);
	ps_const_buf->Half[2] = static_cast<float>(half[2]);
	ps_const_buf->Half[3] = static_cast<float>(pow(max(1.0 - Dot(eye, half), 0.0), 5.0));
}

static SClass* MakeObjImpl(SClass* me_, size_t size, const void* binary)
{
	SObj* me2 = reinterpret_cast<SObj*>(me_);
	me2->ElementKinds = NULL;
	me2->Elements = NULL;
	IdentityFloat(me2->Mat);
	IdentityFloat(me2->NormMat);
	{
		Bool correct = True;
		U32* idces = NULL;
		U8* vertices = NULL;
		const U8* buf = static_cast<const U8*>(binary);
		for (; ; )
		{
			size_t ptr = 0;
			if (ptr + sizeof(int) * 3 > size)
			{
				correct = False;
				break;
			}

			int version = *reinterpret_cast<const int*>(buf + ptr);
			UNUSED(version);
			ptr += sizeof(int);
			ASSERT(version == 1);
			me2->Format = static_cast<EFormat>(*reinterpret_cast<const int*>(buf + ptr));
			ptr += sizeof(int);
			int vertex_size = 8;
			int vertex_size_aligned = 8;
			if ((me2->Format & Format_HasJoint) != 0)
			{
				vertex_size += JointInfluenceMax;
				vertex_size_aligned += JointInfluenceMaxAligned;
			}
			if ((me2->Format & Format_HasTangent) != 0)
			{
				vertex_size += 3;
				vertex_size_aligned += 3;
			}
			int joint_influence_size = 0;
			int joint_influence_size_aligned = 0;
			if ((me2->Format & Format_HasJoint) != 0)
			{
				joint_influence_size += JointInfluenceMax;
				joint_influence_size_aligned += JointInfluenceMaxAligned;
			}

			me2->ElementNum = *reinterpret_cast<const int*>(buf + ptr);
			ptr += sizeof(int);
			if (me2->ElementNum < 0)
			{
				correct = False;
				break;
			}
			me2->ElementKinds = static_cast<int*>(AllocMem(sizeof(int) * static_cast<size_t>(me2->ElementNum)));
			me2->Elements = static_cast<void**>(AllocMem(sizeof(void*) * static_cast<size_t>(me2->ElementNum)));
			for (int i = 0; i < me2->ElementNum; i++)
			{
				if (ptr + sizeof(int) > size)
				{
					correct = False;
					break;
				}
				me2->ElementKinds[i] = *reinterpret_cast<const int*>(buf + ptr);
				ptr += sizeof(int);
				switch (me2->ElementKinds[i])
				{
					case 0: // Polygon.
						{
							SObj::SPolygon* element = static_cast<SObj::SPolygon*>(AllocMem(sizeof(SObj::SPolygon)));
							element->VertexBuf = NULL;
							element->Joints = NULL;
							me2->Elements[i] = element;
							if (ptr + sizeof(int) > size)
							{
								correct = False;
								break;
							}
							element->VertexNum = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							idces = static_cast<U32*>(AllocMem(sizeof(U32) * static_cast<size_t>(element->VertexNum)));
							if (ptr + sizeof(U32) * static_cast<size_t>(element->VertexNum) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < element->VertexNum; j++)
							{
								idces[j] = *reinterpret_cast<const U32*>(buf + ptr);
								ptr += sizeof(U32);
							}
							if (ptr + sizeof(int) > size)
							{
								correct = False;
								break;
							}
							int idx_num = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							vertices = static_cast<U8*>(AllocMem((sizeof(float) * vertex_size_aligned + sizeof(int) * joint_influence_size_aligned) * static_cast<size_t>(idx_num)));
							U8* ptr2 = vertices;
							if (ptr + (sizeof(float) * vertex_size + sizeof(int) * joint_influence_size) * static_cast<size_t>(idx_num) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < idx_num; j++)
							{
								for (int k = 0; k < vertex_size; k++)
								{
									*reinterpret_cast<float*>(ptr2) = *reinterpret_cast<const float*>(buf + ptr);
									ptr += sizeof(float);
									ptr2 += sizeof(float);
								}
								for (int k = 0; k < vertex_size_aligned - vertex_size; k++)
								{
									*reinterpret_cast<float*>(ptr2) = 0.0f;
									ptr2 += sizeof(float);
								}
								for (int k = 0; k < joint_influence_size; k++)
								{
									*reinterpret_cast<int*>(ptr2) = *reinterpret_cast<const int*>(buf + ptr);
									ptr += sizeof(int);
									ptr2 += sizeof(int);
								}
								for (int k = 0; k < joint_influence_size_aligned - joint_influence_size; k++)
								{
									*reinterpret_cast<int*>(ptr2) = 0;
									ptr2 += sizeof(int);
								}
							}
							element->VertexBuf = MakeVertexBuf((sizeof(float) * vertex_size_aligned + sizeof(int) * joint_influence_size_aligned) * static_cast<size_t>(idx_num), vertices, sizeof(float) * vertex_size_aligned + sizeof(int) * joint_influence_size_aligned, sizeof(U32) * static_cast<size_t>(element->VertexNum), idces);
							if (ptr + sizeof(int) * 3 > size)
							{
								correct = False;
								break;
							}
							element->JointNum = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							element->Begin = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							element->End = *reinterpret_cast<const int*>(buf + ptr);
							ptr += sizeof(int);
							element->Joints = static_cast<float(*)[4][4]>(AllocMem(sizeof(float[4][4]) * static_cast<size_t>(element->JointNum * (element->End - element->Begin + 1))));
							if (ptr + sizeof(float[4][4]) * static_cast<size_t>(element->JointNum * (element->End - element->Begin + 1)) > size)
							{
								correct = False;
								break;
							}
							for (int j = 0; j < element->JointNum; j++)
							{
								for (int k = 0; k < element->End - element->Begin + 1; k++)
								{
									for (int l = 0; l < 4; l++)
									{
										for (int m = 0; m < 4; m++)
										{
											element->Joints[j * (element->End - element->Begin + 1) + k][l][m] = *reinterpret_cast<const float*>(buf + ptr);
											ptr += sizeof(float);
										}
									}
								}
							}
						}
						break;
					default:
						THROW(0xe9170008);
						break;
				}
				if (!correct)
					break;
			}
			break;
		}
		if (vertices != NULL)
			FreeMem(vertices);
		if (idces != NULL)
			FreeMem(idces);
		if (!correct)
		{
			_objFin(me_);
			THROW(0xe9170008);
			return NULL;
		}
	}
	return me_;
}
