#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12Buffer.h"
#include "Utilities/Log.h"

const int g_vertexCount = 32;

// Where are these type defined ???
static
DXGI_FORMAT getFormat(u8 type, u8 size)
{
	/*static const u32 gl_types[] =
	{
	GL_SHORT,
	GL_FLOAT,
	GL_HALF_FLOAT,
	GL_UNSIGNED_BYTE,
	GL_SHORT,
	GL_FLOAT, // Needs conversion
	GL_UNSIGNED_BYTE,
	};

	static const bool gl_normalized[] =
	{
	GL_TRUE,
	GL_FALSE,
	GL_FALSE,
	GL_TRUE,
	GL_FALSE,
	GL_TRUE,
	GL_FALSE,
	};*/
	static const DXGI_FORMAT typeX1[] =
	{
		DXGI_FORMAT_R16_SNORM,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R8_UINT
	};
	static const DXGI_FORMAT typeX2[] =
	{
		DXGI_FORMAT_R16G16_SNORM,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R8G8_UINT
	};
	static const DXGI_FORMAT typeX3[] =
	{
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT
	};
	static const DXGI_FORMAT typeX4[] =
	{
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT
	};

	switch (size)
	{
	case 1:
		return typeX1[type];
	case 2:
		return typeX2[type];
	case 3:
		return typeX3[type];
	case 4:
		return typeX4[type];
	}
}

std::vector<D3D12_INPUT_ELEMENT_DESC> getIALayout(ID3D12Device *device, bool indexedDraw, const RSXVertexData *vertexData)
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> result;
	u32 offset_list[g_vertexCount];
	u32 cur_offset = 0;

	const u32 data_offset = indexedDraw ? 0 : 1;// m_draw_array_first;

	for (u32 i = 0; i < g_vertexCount; ++i)
	{
		offset_list[i] = cur_offset;

		if (!vertexData[i].IsEnabled()) continue;
		const size_t item_size = vertexData[i].GetTypeSize() * vertexData[i].size;
		const size_t data_size = vertexData[i].data.size() - data_offset * item_size;
		cur_offset += data_size;
	}

#if	DUMP_VERTEX_DATA
	rFile dump("VertexDataArray.dump", rFile::write);
#endif

	for (u32 i = 0; i < g_vertexCount; ++i)
	{
		if (!vertexData[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch (m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S1:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_F:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_SF:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_UB:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_S32K:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

			// case CELL_GCM_VERTEX_CMP:

		case CELL_GCM_VERTEX_UB256:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		default:
			LOG_ERROR(HLE, "Bad cv type! %d", m_vertex_data[i].type);
			return;
		}

		dump.Write("\n");
#endif

		if (vertexData[i].type < 1 || vertexData[i].type > 7)
		{
			LOG_ERROR(RSX, "GLGSRender::EnableVertexData: Bad vertex data type (%d)!", vertexData[i].type);
		}

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
		/*		if (!m_vertex_data[i].addr)
		{
		switch (m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S32K:
		case CELL_GCM_VERTEX_S1:
		switch (m_vertex_data[i].size)
		{
		case 1: glVertexAttrib1s(i, (GLshort&)m_vertex_data[i].data[0]); break;
		case 2: glVertexAttrib2sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
		case 3: glVertexAttrib3sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
		case 4: glVertexAttrib4sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
		}
		break;

		case CELL_GCM_VERTEX_F:
		switch (m_vertex_data[i].size)
		{
		case 1: glVertexAttrib1f(i, (GLfloat&)m_vertex_data[i].data[0]); break;
		case 2: glVertexAttrib2fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
		case 3: glVertexAttrib3fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
		case 4: glVertexAttrib4fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
		}
		break;

		case CELL_GCM_VERTEX_CMP:
		case CELL_GCM_VERTEX_UB:
		glVertexAttrib4ubv(i, (GLubyte*)&m_vertex_data[i].data[0]);
		break;
		}

		checkForGlError("glVertexAttrib");
		}
		else*/
		{
			IAElement.SemanticName = "TEXCOORD";
			IAElement.SemanticIndex = i;
			IAElement.Format = getFormat(vertexData[i].type - 1, vertexData[i].size);

			IAElement.AlignedByteOffset = offset_list[i];
		}
		result.push_back(IAElement);
	}
	return result;
}

#endif