//
// knobj_maker.exe
//
// (C)Kuina-chan
//

// This source code requires the FBX SDK 2016 for compilation.
// This program can read 'FBX 200900 binary' files outputted with the default option.

#pragma warning(disable: 4127)
#pragma warning(disable: 4996)

#pragma comment(lib, "libfbxsdk-mt.lib")

#include "main.h"
#include "fbxsdk.h"

#include <fcntl.h>
#include <io.h>

// 0 = 'Ja', 1 = 'En'.
#define LANG (0)

static const Bool Mirror = False; // Invert the Z axis.
static const int JointInfluenceMax = 2;

struct SPoint
{
	Bool Unique;
	double PosX, PosY, PosZ;
	double NormalX, NormalY, NormalZ;
	double TangentX, TangentY, TangentZ;
	// double BinormalX, BinormalY, BinormalZ;
	double TexU, TexV;
	double JointWeight[JointInfluenceMax];
	int Joint[JointInfluenceMax];
};

enum EFormat
{
	Format_HasTangent = 0x01,
	Format_HasJoint = 0x02,
};

static FbxManager* Manager = NULL;
static FbxScene* Scene = NULL;
static FILE* FilePtr = NULL;
static int Format = 0;

static void Warn(const char* msg_ja, const char* msg_en);
static void Normalize(double* vec);
static void CalcTangent(double* tangents, double* binormals, const double* normals, const double* ps, const double* uvs);
static SPoint MirrorPoint(const SPoint* p);
static Bool Same(double a, double b);
static Bool CmpPoints(const SPoint* a, const SPoint* b);
static void WriteInt(int n);
static void WriteFloat(double n);
static void WriteNode(FbxNode* root);

static void Warn(const char* msg_ja, const char* msg_en)
{
#if LANG == 0
	printf("%s\n", msg_ja);
#else
	printf("%s\n", msg_en);
#endif
}

static void Normalize(double* vec)
{
	double len = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	if (len == 0.0)
		return;
	vec[0] /= len;
	vec[1] /= len;
	vec[2] /= len;
}

static void CalcTangent(double* tangents, double* binormals, const double* normals, const double* ps, const double* uvs)
{
	for (int i = 0; i < 3; i++)
	{
		double edge0[3] = { ps[3 * 1 + i] - ps[3 * 0 + i], uvs[2 * 1 + 0] - uvs[2 * 0 + 0], uvs[2 * 1 + 1] - uvs[2 * 0 + 1] };
		double edge1[3] = { ps[3 * 2 + i] - ps[3 * 0 + i], uvs[2 * 2 + 0] - uvs[2 * 0 + 0], uvs[2 * 2 + 1] - uvs[2 * 0 + 1] };

		// 'edge0' x 'edge1'.
		double cross[3];
		cross[0] = edge0[1] * edge1[2] - edge0[2] * edge1[1];
		cross[1] = edge0[2] * edge1[0] - edge0[0] * edge1[2];
		cross[2] = edge0[0] * edge1[1] - edge0[1] * edge1[0];

		Normalize(cross);
		if (cross[0] == 0.0)
			cross[0] = 1.0;
		double tan_factor = -cross[1] / cross[0];
		tangents[0 + i] = tan_factor;
		tangents[3 + i] = tan_factor;
		tangents[6 + i] = tan_factor;
	}

	for (int i = 0; i < 3; i++)
	{
		// Orthonormalize to normal.
		double dot = tangents[i * 3 + 0] * normals[i * 3 + 0] + tangents[i * 3 + 1] * normals[i * 3 + 1] + tangents[i * 3 + 2] * normals[i * 3 + 2];
		tangents[i * 3 + 0] -= normals[i * 3 + 0] * dot;
		tangents[i * 3 + 1] -= normals[i * 3 + 1] * dot;
		tangents[i * 3 + 2] -= normals[i * 3 + 2] * dot;

		Normalize(&tangents[i * 3]);

		binormals[i * 3 + 0] = tangents[i * 3 + 1] * normals[i * 3 + 2] - tangents[i * 3 + 2] * normals[i * 3 + 1];
		binormals[i * 3 + 1] = tangents[i * 3 + 2] * normals[i * 3 + 0] - tangents[i * 3 + 0] * normals[i * 3 + 2];
		binormals[i * 3 + 2] = tangents[i * 3 + 0] * normals[i * 3 + 1] - tangents[i * 3 + 1] * normals[i * 3 + 0];
	}
}

static SPoint MirrorPoint(const SPoint* p)
{
	SPoint result = *p;
	result.PosZ = -result.PosZ;
	result.NormalZ = -result.NormalZ;
	// result.BinormalZ = -result.BinormalZ;
	result.TangentZ = -result.TangentZ;
	result.TexV = result.TexV * 2.0 - 1.0;
	return result;
}

static Bool Same(double a, double b)
{
	U64 i1 = *(U64*)&a;
	U64 i2 = *(U64*)&b;
	S64 diff;
	if ((i1 >> 63) != (i2 >> 63))
		return a == b;
	diff = (S64)(i1 - i2);
	return -24 <= diff && diff <= 24;
}

static Bool CmpPoints(const SPoint* a, const SPoint* b)
{
	U32 flag = 1;
	flag &= Same(a->PosX, b->PosX);
	flag &= Same(a->PosY, b->PosY);
	flag &= Same(a->PosZ, b->PosZ);
	flag &= Same(a->NormalX, b->NormalX);
	flag &= Same(a->NormalY, b->NormalY);
	flag &= Same(a->NormalZ, b->NormalZ);
	flag &= Same(a->TangentX, b->TangentX);
	flag &= Same(a->TangentY, b->TangentY);
	flag &= Same(a->TangentZ, b->TangentZ);
	/*
	flag &= Same(a->BinormalX, b->BinormalX);
	flag &= Same(a->BinormalY, b->BinormalY);
	flag &= Same(a->BinormalZ, b->BinormalZ);
	*/
	flag &= Same(a->TexU, b->TexU);
	flag &= Same(a->TexV, b->TexV);
	for (int i = 0; i < JointInfluenceMax; i++)
	{
		flag &= Same(a->JointWeight[i], b->JointWeight[i]);
		flag &= Same(a->Joint[i], b->Joint[i]);
	}
	return flag != 0;
}

static void WriteInt(int n)
{
	fwrite(&n, 1, sizeof(n), FilePtr);
}

static void WriteFloat(double n)
{
	float n2 = static_cast<float>(n);
	fwrite(&n2, 1, sizeof(n2), FilePtr);
}

static void WriteNode(FbxNode* root)
{
	WriteInt(1); // Version.
	WriteInt(Format);

	int child_num = root->GetChildCount();
	{
		int cnt = 0;
		for (int child = 0; child < child_num; child++)
		{
			FbxNode* node = root->GetChild(child);
			FbxNodeAttribute* attr = node->GetNodeAttribute();
			if (attr == NULL)
			{
				Warn("アトリビュートの無いノードがありました。", "There is a node which has no attributes.");
				continue;
			}
			switch (attr->GetAttributeType())
			{
			case FbxNodeAttribute::eMesh:
				cnt++;
				break;
			case FbxNodeAttribute::eSkeleton:
			case FbxNodeAttribute::eCamera:
			case FbxNodeAttribute::eLight:
				// Do not count.
				break;
			default:
				break;
			}
		}
		WriteInt(cnt);
	}
	for (int child = 0; child < child_num; child++)
	{
		FbxNode* node = root->GetChild(child);
		FbxNodeAttribute* attr = node->GetNodeAttribute();
		if (attr == NULL)
			continue;
		switch (attr->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
		{
			WriteInt(0); // Polygon.
			FbxMesh* mesh = static_cast<FbxMesh*>(attr);

			// Get number of vertices.
			int vertex_num = mesh->GetPolygonCount() * 3;
			WriteInt(vertex_num);
			int* indices = static_cast<int*>(malloc(sizeof(int) * static_cast<size_t>(vertex_num)));
			for (int i = 0; i < vertex_num / 3; i++)
			{
				if (mesh->GetPolygonSize(i) != 3)
				{
					Warn("頂点が3個以外のポリゴンがありました。", "There is a polygon which has non-three vertices.");
					for (int j = 0; j < 3; j++)
						indices[i * 3 + j] = 0;
					continue;
				}
				for (int j = 0; j < 3; j++)
					indices[i * 3 + j] = mesh->GetPolygonVertex(i, j);
			}

			// Get a normal.
			FbxGeometryElementNormal* normal = mesh->GetElementNormal();
			FbxGeometryElementNormal::EMappingMode normal_mapping = normal->GetMappingMode();
			FbxGeometryElementNormal::EReferenceMode normal_ref = normal->GetReferenceMode();

			// Get a UV.
			FbxGeometryElementUV* uv = mesh->GetElementUV();
			FbxGeometryElementNormal::EMappingMode uv_mapping = uv->GetMappingMode();
			FbxGeometryElementNormal::EReferenceMode uv_ref = uv->GetReferenceMode();

			// Get joints.
			int ctrl_point_num = mesh->GetControlPointsCount();
			int joint_num = 0;
			double* joints = NULL;
			{
				int skin_num = mesh->GetDeformerCount(FbxDeformer::eSkin);
				if (skin_num != 0)
				{
					FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
					joint_num = skin->GetClusterCount();
					joints = static_cast<double*>(malloc(sizeof(double) * static_cast<size_t>(ctrl_point_num * joint_num)));
					for (int i = 0; i < ctrl_point_num * joint_num; i++)
						joints[i] = 0.0;
					for (int i = 0; i < joint_num; i++)
					{
						FbxCluster* cluster = skin->GetCluster(i);
						int cluster_point_num = cluster->GetControlPointIndicesCount();
						int* cluster_points = cluster->GetControlPointIndices();
						double* cluster_weights = cluster->GetControlPointWeights();
						for (int j = 0; j < cluster_point_num; j++)
							joints[cluster_points[j] * joint_num + i] = cluster_weights[j];
					}
				}
			}

			// Get vertices.
			SPoint* points = static_cast<SPoint*>(malloc(sizeof(SPoint) * static_cast<size_t>(vertex_num)));
			FbxVector4* vertices = mesh->GetControlPoints();
			for (int i = 0; i < vertex_num; i++)
			{
				points[i].Unique = True;

				points[i].PosX = vertices[indices[i]][0];
				points[i].PosY = vertices[indices[i]][1];
				points[i].PosZ = vertices[indices[i]][2];

				if (normal == NULL)
				{
					Warn("法線の無い頂点があります。", "There is a vertex which has no normals.");
					points[i].NormalX = 0.0f;
					points[i].NormalY = 1.0f;
					points[i].NormalZ = 0.0f;
				}
				else
				{
					int v_idx = normal_mapping == FbxGeometryElementNormal::eByPolygonVertex ? i : indices[i];
					int idx = normal_ref == FbxGeometryElementNormal::eDirect ? v_idx : normal->GetIndexArray().GetAt(v_idx);
					FbxVector4 vec = normal->GetDirectArray().GetAt(idx);
					points[i].NormalX = vec[0];
					points[i].NormalY = vec[1];
					points[i].NormalZ = vec[2];
				}

				if (uv == NULL)
				{
					Warn("UVの無い頂点があります。", "There is a vertex which has no UVs.");
					points[i].TexU = 0.0f;
					points[i].TexV = 0.0f;
				}
				else
				{
					int v_idx = uv_mapping == FbxGeometryElementNormal::eByPolygonVertex ? i : indices[i];
					int idx = uv_ref == FbxGeometryElementNormal::eDirect ? v_idx : uv->GetIndexArray().GetAt(v_idx);
					FbxVector2 vec = uv->GetDirectArray().GetAt(idx);
					points[i].TexU = vec[0];
					points[i].TexV = 1.0 - vec[1]; // Top = 0.0, Bottom is 1.0
				}

				if (joints == NULL || (Format & Format_HasJoint) == 0)
				{
					for (int j = 0; j < JointInfluenceMax; j++)
					{
						points[i].JointWeight[j] = 0.0;
						points[i].Joint[j] = 0;
					}
				}
				else
				{
					for (int j = 0; j < JointInfluenceMax; j++)
					{
						// Get four bones in descending order of weights by selection sort.
						int joint = -1;
						double max = 0.0;
						for (int k = 0; k < joint_num; k++)
						{
							if (max < joints[indices[i] * joint_num + k])
							{
								{
									Bool skip = False;
									for (int l = 0; l < j; l++)
									{
										if (points[i].Joint[l] == k)
										{
											skip = True;
											break;
										}
									}
									if (skip)
										continue;
								}
								joint = k;
								max = joints[indices[i] * joint_num + k];
							}
						}
						if (max == 0.0)
						{
							points[i].JointWeight[j] = 0.0;
							points[i].Joint[j] = 0;
						}
						else
						{
							points[i].JointWeight[j] = joints[indices[i] * joint_num + joint];
							points[i].Joint[j] = joint;
						}
					}
					// Normalize weights.
					{
						double sum = 0.0;
						for (int j = 0; j < JointInfluenceMax; j++)
							sum += points[i].JointWeight[j];
						if (sum == 0.0)
							Warn("ジョイントの重みの和は0以外でなければなりません。", "The sum of weights of joints must not be zero.");
						for (int j = 0; j < JointInfluenceMax; j++)
							points[i].JointWeight[j] /= sum;
					}
				}
			}

			// Calculate the tangent and the binormal.
			if ((Format & Format_HasTangent) != 0)
			{
				for (int i = 0; i < vertex_num / 3; i++)
				{
					double tangents[9], binormals[9];
					double normals[9] =
					{
						points[i * 3 + 0].NormalX, points[i * 3 + 0].NormalY, points[i * 3 + 0].NormalZ,
						points[i * 3 + 1].NormalX, points[i * 3 + 1].NormalY, points[i * 3 + 1].NormalZ,
						points[i * 3 + 2].NormalX, points[i * 3 + 2].NormalY, points[i * 3 + 2].NormalZ,
					};
					double ps[9] =
					{
						points[i * 3 + 0].PosX, points[i * 3 + 0].PosY, points[i * 3 + 0].PosZ,
						points[i * 3 + 1].PosX, points[i * 3 + 1].PosY, points[i * 3 + 1].PosZ,
						points[i * 3 + 2].PosX, points[i * 3 + 2].PosY, points[i * 3 + 2].PosZ,
					};
					double uvs[6] =
					{
						points[i * 3 + 0].TexU, points[i * 3 + 0].TexV,
						points[i * 3 + 1].TexU, points[i * 3 + 1].TexV,
						points[i * 3 + 2].TexU, points[i * 3 + 2].TexV,
					};
					CalcTangent(tangents, binormals, normals, ps, uvs);
					points[i * 3 + 0].TangentX = tangents[0];
					points[i * 3 + 0].TangentY = tangents[1];
					points[i * 3 + 0].TangentZ = tangents[2];
					points[i * 3 + 1].TangentX = tangents[3];
					points[i * 3 + 1].TangentY = tangents[4];
					points[i * 3 + 1].TangentZ = tangents[5];
					points[i * 3 + 2].TangentX = tangents[6];
					points[i * 3 + 2].TangentY = tangents[7];
					points[i * 3 + 2].TangentZ = tangents[8];
					/*
					points[i * 3 + 0].BinormalX = binormals[0];
					points[i * 3 + 0].BinormalY = binormals[1];
					points[i * 3 + 0].BinormalZ = binormals[2];
					points[i * 3 + 1].BinormalX = binormals[3];
					points[i * 3 + 1].BinormalY = binormals[4];
					points[i * 3 + 1].BinormalZ = binormals[5];
					points[i * 3 + 2].BinormalX = binormals[6];
					points[i * 3 + 2].BinormalY = binormals[7];
					points[i * 3 + 2].BinormalZ = binormals[8];
					*/
				}
			}
			else
			{
				for (int i = 0; i < vertex_num; i++)
				{
					points[i].TangentX = 0.0;
					points[i].TangentY = 0.0;
					points[i].TangentZ = 0.0;
				}
			}

			// Invert the Z axis.
			if (Mirror)
			{
				for (int i = 0; i < vertex_num / 3; i++)
				{
					points[i * 3] = MirrorPoint(&points[i * 3]);
					SPoint p1 = MirrorPoint(&points[i * 3 + 1]);
					SPoint p2 = MirrorPoint(&points[i * 3 + 2]);
					points[i * 3 + 1] = p2;
					points[i * 3 + 2] = p1;
				}
			}

			// Write the indices.
			int idx_num = 0;
			for (int i = 0; i < vertex_num; i++)
			{
				int idx = 0;
				for (int j = 0; j < i; j++)
				{
					if (!points[j].Unique)
						continue;
					if (CmpPoints(&points[i], &points[j]))
					{
						points[i].Unique = False;
						break;
					}
					idx++;
				}
				if (points[i].Unique)
					idx_num++;
				WriteInt(idx);
			}

			// Write the vertices.
			WriteInt(idx_num);
			{
				for (int i = 0; i < vertex_num; i++)
				{
					if (!points[i].Unique)
						continue;
					WriteFloat(points[i].PosX);
					WriteFloat(points[i].PosY);
					WriteFloat(points[i].PosZ);

					WriteFloat(points[i].NormalX);
					WriteFloat(points[i].NormalY);
					WriteFloat(points[i].NormalZ);

					if ((Format & Format_HasTangent) != 0)
					{
						WriteFloat(points[i].TangentX);
						WriteFloat(points[i].TangentY);
						WriteFloat(points[i].TangentZ);
					}

					/*
					WriteFloat(points[i].BinormalX);
					WriteFloat(points[i].BinormalY);
					WriteFloat(points[i].BinormalZ);
					*/

					WriteFloat(points[i].TexU);
					WriteFloat(points[i].TexV);

					if ((Format & Format_HasJoint) != 0)
					{
						for (int j = 0; j < JointInfluenceMax; j++)
							WriteFloat(points[i].JointWeight[j]);
						for (int j = 0; j < JointInfluenceMax; j++)
							WriteInt(points[i].Joint[j]);
					}
				}
			}

			// Write the joints.
			WriteInt(joint_num);
			if (joints == NULL)
			{
				WriteInt(0);
				WriteInt(1);
			}
			else
			{
				FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
				FbxTime::EMode time_mode = Scene->GetGlobalSettings().GetTimeMode();
				if (time_mode != FbxTime::eFrames30 && time_mode != FbxTime::eFrames60)
				{
					Warn("タイムモードは30FPSか60FPSでなければなりません。", "The time mode must be 30 FPS or 60 FPS.");
					time_mode = FbxTime::eFrames60;
				}
				int begin = 0, end = 0;
				FbxTime period;
				{
					period.SetTime(0, 0, 0, 1, 0, 0, time_mode);
					FbxArray<FbxString*> array_;
					Scene->FillAnimStackNameArray(array_);
					int num = array_.GetCount();
					for (int i = 0; i < num; i++)
					{
						FbxTakeInfo* info = Scene->GetTakeInfo(*(array_[i]));
						if (info == NULL)
						{
							Warn("アニメーションが見つかりません。", "No animations were found.");
							begin = 0;
							end = 0;
						}
						else
						{
							begin = static_cast<int>(info->mLocalTimeSpan.GetStart().Get() / period.Get());
							end = static_cast<int>(info->mLocalTimeSpan.GetStop().Get() / period.Get());
						}
					}
					FbxArrayDelete(array_);
				}

				WriteInt(begin - 1);
				WriteInt(end - 1);
				for (int i = 0; i < joint_num; i++)
				{
					FbxCluster* cluster = skin->GetCluster(i);

					// Inverse matrix of initial pose.
					FbxAMatrix default_mat;
					cluster->GetTransformLinkMatrix(default_mat);
					default_mat = default_mat.Inverse();

					// Write animations.
					for (int j = begin; j <= end; j++)
					{
						FbxAMatrix mat;
						mat = cluster->GetLink()->GetAnimationEvaluator()->GetNodeGlobalTransform(cluster->GetLink(), period * j);
						mat = mat * default_mat;
						for (int k = 0; k < 4; k++)
						{
							for (int l = 0; l < 4; l++)
								WriteFloat((Mirror && (k == 2 || l == 2) && k != l ? -1.0 : 1.0) * mat[k][l]);
						}
					}
				}
			}

			free(joints);
			free(points);
			free(indices);
		}
		if (node->GetChildCount() != 0)
			Warn("メッシュに子ノードがありましたが、読み込めません。", "There is a mesh which has child nodes, but it cannot be read.");
		break;
		case FbxNodeAttribute::eSkeleton:
			// Do nothing.
			break;
		case FbxNodeAttribute::eCamera:
			// TODO: Write a camera.
			break;
		case FbxNodeAttribute::eLight:
			// TODO: Write a light.
			break;
		default:
			break;
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		const char* usage = "Usage: knobj_maker format [fbx file1] [fbx file2] [...]";
		Warn(usage, usage);
		return 0;
	}

	Manager = FbxManager::Create();
	if (Manager == NULL)
	{
		Warn("knobj_maker の初期化に失敗しました。", "knobj_maker initialization failed.");
		return 1;
	}
	Manager->SetIOSettings(FbxIOSettings::Create(Manager, IOSROOT));
	Manager->LoadPluginsDirectory(FbxGetApplicationDirectory().Buffer());
	Scene = FbxScene::Create(Manager, "");
	if (Scene == NULL)
	{
		Warn("knobj_maker の初期化に失敗しました。", "knobj_maker initialization failed.");
		return 1;
	}

	Format = atoi(argv[1]);

	for (int i = 2; i < argc; i++)
	{
		Warn("コンバート開始です:", "Conversion starts:");
		printf("%s\n", argv[i]);

		if (!PathFileExistsA(argv[i]))
		{
			Warn("ファイルが見つかりません。", "File not found.");
			continue;
		}
		{
			char path[1024];
			char* pos = strrchr(argv[i], '.');
			if (pos == NULL)
			{
				strcpy(path, argv[i]);
			}
			else
			{
				strncpy(path, argv[i], pos - argv[i]);
				path[pos - argv[i]] = '\0';
			}
			strcat(path, ".knobj");
			FilePtr = fopen(path, "wb");
			if (FilePtr == NULL)
			{
				Warn("ファイルの作成に失敗しました。", "File creation failed.");
				continue;
			}
		}

		{
			FbxImporter* importer = FbxImporter::Create(Manager, "");
			importer->Initialize(argv[i], -1, Manager->GetIOSettings());
			importer->Import(Scene);
			importer->Destroy();
		}

		{
			FbxNode* root = Scene->GetRootNode();
			if (root->GetNodeAttribute() != NULL)
				Warn("アトリビュートの無いノードがありました。", "There is a node which has no attributes.");
			else
				WriteNode(root);
		}

		fclose(FilePtr);
	}

	Manager->Destroy();

	printf("Done.\n");
	return 0;
}
