#include "pch.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/encodedstream.h>    // AutoUTFInputStream
#include <rapidjson/prettywriter.h>

#define CHECK(x) {if(!(x)) assert(x); goto Exit0;}

using namespace rapidjson;

bool ReplaceString(Document& d);

int main(int argc, char** argv)
{
	UTFType fileUTFType;
	bool fileHasBOM;
	Document d;         // Document is GenericDocument<UTF8<> > 

	// Read file
	{
		FILE* fp = nullptr;
		fopen_s(&fp, argv[1], "rb"); // non-Windows use "r"

		char buffer[65536];
		FileReadStream bis(fp, buffer, sizeof(buffer));

		AutoUTFInputStream<unsigned, FileReadStream> eis(bis);  // wraps bis into eis

		d.ParseStream<0, AutoUTF<unsigned> >(eis); // This parses any UTF file into UTF-8 in memory

		fclose(fp);

		if (d.HasParseError())
		{
			printf("parse json file error(%d)\n", d.GetParseError());
			goto Exit0;
		}

		fileUTFType = eis.GetType();
		fileHasBOM = eis.HasBOM();
	}

	// Replace string
	{
		if (!ReplaceString(d))
		{
			printf("replace string error\n");
			goto Exit0;
		}
	}

	// Write file
	{
		std::string filename = argv[1];
		filename = filename.insert(filename.find_last_of('.'), "_replaced");

		FILE* fp = nullptr;
		fopen_s(&fp, filename.c_str(), "wb"); // non-Windows use "w"

		char write_buffer[65536];
		FileWriteStream os(fp, write_buffer, sizeof(write_buffer));

		typedef AutoUTFOutputStream<unsigned, FileWriteStream> OutputStream;
		OutputStream eos(os, fileUTFType, fileHasBOM);

		PrettyWriter<OutputStream, UTF8<>, UTF16LE<> > writer(eos); // 记得要填入eos，官网的例子里漏掉了
		d.Accept(writer);

		fclose(fp);
	}

Exit0:
	system("pause");
	return -1;
}

bool IsHeadMesh(const char* Name)
{
	if (!strcmp(Name, "mod_hd_hf1_01"))
		return true;

	if (!strcmp(Name, "mod_hd_hf2_01"))
		return true;

	if (!strcmp(Name, "mod_hd_hf2_02"))
		return true;

	if (!strcmp(Name, "mod_hd_hm1_01"))
		return true;

	if (!strcmp(Name, "mod_hd_hm2_01"))
		return true;

	return false;
}

const char* MeshSlotString = 
"{\
	\"SlotPart\": \"eFaceSlot\",\
	\"CustomSlotName\" : \"root\",\
	\"OverrideTransform\" : false,\
	\"Location\" :\
	{\
		\"X\": 0,\
		\"Y\" : 0,\
		\"Z\" : 0\
	},\
	\"Rotation\":\
	{\
		\"Pitch\": 0,\
		\"Yaw\" : 0,\
		\"Roll\" : 0\
	},\
	\"Scale\":\
	{\
		\"X\": 0,\
		\"Y\" : 0,\
		\"Z\" : 0\
	},\
	\"ChildSocket\": \"\"\
}";

Document MakeMeshSlotValue(Document& doc, Value& SkinMeshValue)
{
	Document subDoc(&doc.GetAllocator());
	subDoc.Parse(MeshSlotString);
	if (subDoc.HasParseError())
	{
		printf("parse json file error(%d)\n", subDoc.GetParseError());
	}
	subDoc.AddMember("MeshPrefab", Value(StringRef(SkinMeshValue["SkinMeshPrefab"].GetString())), doc.GetAllocator());

	return subDoc;
}

bool ReplaceString(Document& doc)
{
	for (SizeType rootArrayIndex = 0; rootArrayIndex < doc.Size(); rootArrayIndex++)
	{
		auto& Row = doc[rootArrayIndex];

		// Find SkinMesh
		bool found = false;
		Value::ValueIterator itr;
		auto SkinMeshs = Row["SkinMeshs"].GetArray();
		for (itr = SkinMeshs.Begin(); itr != SkinMeshs.End(); ++itr)
		{
			const char* PrefabName = (*itr)["SkinMeshPrefab"].GetString();
			if (IsHeadMesh(PrefabName))
			{
				found = true;
				break;
			}
		}

		if (!found)
			continue;

		// Add MeshSlot
		auto& MeshSlots = Row["MeshSlots"];
		MeshSlots.GetArray().PushBack(MakeMeshSlotValue(doc, (*itr)), doc.GetAllocator());

		// Remove SkinMesh
		SkinMeshs.Erase(itr);

		printf("replace row %s\n", Row["Name"].GetString());
	}

	return true;
Exit0:
	return false;
}
