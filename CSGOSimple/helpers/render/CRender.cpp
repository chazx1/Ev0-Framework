#include "CRender.h"
#include "..\..\valve_sdk\misc\Color.hpp"
#include "..\..\valve_sdk\math\Vector2D.hpp"
#include "..\..\hooks.hpp"
#include "..\..\VMP.hpp"


#pragma region CRender

static const Vector2D CURSOR_COORDINATES[8][3] =
{
	{ Vector2D(0,0),  Vector2D(2,2), Vector2D(0, 0) },
{ Vector2D(0,3),  Vector2D(12,19), Vector2D(0, 0) }, // ImGuiMouseCursor_Arrow
{ Vector2D(13,0), Vector2D(7,16),  Vector2D(4, 8) }, // ImGuiMouseCursor_TextInput
{ Vector2D(31,0), Vector2D(23,23), Vector2D(11,11) }, // ImGuiMouseCursor_ResizeAll
{ Vector2D(21,0), Vector2D(9,23), Vector2D(5,11) }, // ImGuiMouseCursor_ResizeNS
{ Vector2D(55,18),Vector2D(23, 9), Vector2D(11, 5) }, // ImGuiMouseCursor_ResizeEW
{ Vector2D(73,0), Vector2D(17,17), Vector2D(9, 9) }, // ImGuiMouseCursor_ResizeNESW
{ Vector2D(55,0), Vector2D(17,17), Vector2D(9, 9) }, // ImGuiMouseCursor_ResizeNWSE
};

void CRender::RenderDrawList(PizdaDrawList* list)
{
	const auto vertex_size = list->DrawListVTXSize;
	const auto index_size = list->DrawListIDXSize;
	if (!this->_VertexBuffer || g_VertexBufferSize < vertex_size) {
		if (this->_VertexBuffer)
		{
			this->_VertexBuffer->Release();
			this->_VertexBuffer = nullptr;
		}
		g_VertexBufferSize = vertex_size + 5000;
		if (m_pDevice->CreateVertexBuffer(g_VertexBufferSize * sizeof(D3DTLVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM_VERTEX, D3DPOOL_DEFAULT, &this->_VertexBuffer, NULL) < 0)
			return;
	}
	if (!this->_IndexBuffer || g_IndexBufferSize < index_size) {
		if (this->_IndexBuffer)
		{
			this->_IndexBuffer->Release();
			this->_IndexBuffer = nullptr;
		}
		g_IndexBufferSize = index_size + 10000;
		if (m_pDevice->CreateIndexBuffer(g_IndexBufferSize * sizeof(DWORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &this->_IndexBuffer, NULL) < 0)
			return;
	}
	PD3DTLVERTEX vertex;
	PDWORD idx;
	if (this->_VertexBuffer->Lock(0, (UINT)(vertex_size * sizeof(D3DTLVERTEX)), (void**)&vertex, D3DLOCK_DISCARD) < 0)
		return;
	if (this->_IndexBuffer->Lock(0, (UINT)(index_size * sizeof(DWORD)), (void**)&idx, D3DLOCK_DISCARD) < 0)
		return;
	int sz = 0;
	for (auto& cmd : list->VertexBuffer) {
		vertex->x = cmd.x;
		vertex->y = cmd.y;
		vertex->z = 0.f;
		vertex->dxColor = cmd.dxColor;
		vertex->tu = cmd.tu;
		vertex->tv = cmd.tv;
		vertex++;
	}
	memcpy(idx, list->IndexBuffer.Data, list->IndexBuffer.Size * sizeof(DWORD));
	//idx += cmd._IndexBufferSize;
	this->_VertexBuffer->Unlock();
	this->_IndexBuffer->Unlock();
	m_pDevice->SetStreamSource(0, this->_VertexBuffer, 0, sizeof(D3DTLVERTEX));
	m_pDevice->SetIndices(this->_IndexBuffer);
	m_pDevice->SetFVF(D3DFVF_CUSTOM_VERTEX);
	SetVertexState();
	int vtx_of = 0;
	int idx_of = 0;
	for (auto& cmd : list->commands) {
		m_pDevice->SetTexture(0, cmd._TexturePtr);
		m_pDevice->SetScissorRect(&cmd._ScissorRect);
		m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vtx_of, 0, (UINT)cmd._VertexBufferSize, idx_of, cmd._IndexBufferSize / 3);
		idx_of += cmd._IndexBufferSize;
		vtx_of += cmd._VertexBufferSize;
	}
}

void CRender::Initialize(IDirect3DDevice9* pDevice) {
	MUT("CRender::CRender(IDirect3DDevice9* pDevice)");
	LPDIRECT3DTEXTURE9*	Images = new LPDIRECT3DTEXTURE9[256];
	m_pDevice = pDevice;

	if (!CreateObject())
	{
		MessageBoxA(0, "Fail to create d3d9 objects", "Error", MB_OK | MB_ICONERROR);
		TerminateProcess(GetCurrentProcess(), 0);
	}
	int i = 0;
	static string imagenames[] = {
		"item_assaultsuit","item_defuser","item_kevlar","weapon_ak47",
		"weapon_aug","weapon_awp","weapon_bayonet","weapon_bizon","weapon_c4","weapon_cz75a","weapon_deagle","weapon_decoy",
		"weapon_elite","weapon_famas","weapon_fiveseven","weapon_flashbang","weapon_g3sg1","weapon_galilar","weapon_glock",
		"weapon_hegrenade","weapon_hkp2000","weapon_incgrenade","weapon_knife","weapon_knife_butterfly","weapon_knife_falchion",
		"weapon_knife_flip","weapon_knife_gut","weapon_knife_karambit","weapon_knife_m9_bayonet","weapon_knife_push",
		"weapon_knife_t","weapon_knife_tactical","weapon_m4a1","weapon_m4a1_silencer","weapon_m249","weapon_mac10","weapon_mag7",
		"weapon_molotov","weapon_mp7","weapon_mp9","weapon_negev","weapon_nova","weapon_p90","weapon_p250","weapon_revolver","weapon_sawedoff",
		"weapon_scar20","weapon_sg556","weapon_smokegrenade","weapon_ssg08","weapon_taser","weapon_tec9","weapon_ump45","weapon_usp_silencer",
		"weapon_xm1014"
	};
	for (auto s : imagenames) {
		string govno = "Weapons/";
		govno += s;
		govno += ".png";
		if (auto ret = D3DXCreateTextureFromFileA(pDevice, govno.c_str(), &Images[i++]) != D3D_OK) {
			MessageBoxA(NULL, string("Failed to load texture: " + s + "!\nError: " + (ret == D3DERR_NOTAVAILABLE ? "NOT AVAILABLE" : (ret == D3DERR_OUTOFVIDEOMEMORY ? "OUT OF VIDEOMEMORY" : (ret == D3DERR_INVALIDCALL ? "INVALID CALL" : (ret == D3DXERR_INVALIDDATA ? "INVALID DATA" : (ret == E_OUTOFMEMORY ? "OUT OF MEMORY" : "UNKNOWN: " + std::to_string(ret))))))).c_str(), 0, 0);
			//ExitProcess(0);
		}
		static map<uint32_t, string> wpns = {
			{ 1,"weapon_deagle" },{ 2,"weapon_elite" },{ 3,"weapon_fiveseven" },{ 4,"weapon_glock" },{ 7,"weapon_ak47" },{ 8,"weapon_aug" },{ 9,"weapon_awp" },{ 10,"weapon_famas" },{ 11,"weapon_g3sg1" },{ 13,"weapon_galilar" },{ 14,"weapon_m249" },{ 16,"weapon_m4a1" },{ 17,"weapon_mac10" },
		{ 19,"weapon_p90" },{ 24,"weapon_ump45" },{ 25,"weapon_xm1014" },{ 26,"weapon_bizon" },{ 27,"weapon_mag7" },{ 28,"weapon_negev" },{ 29,"weapon_sawedoff" },{ 30,"weapon_tec9" },{ 32,"weapon_hkp2000" },{ 33,"weapon_mp7" },{ 34,"weapon_mp9" },{ 35,"weapon_nova" },
		{ 36,"weapon_p250" },{ 38,"weapon_scar20" },{ 39,"weapon_sg556" },{ 40,"weapon_ssg08" },{ 60,"weapon_m4a1_silencer" },{ 61,"weapon_usp_silencer" },{ 63,"weapon_cz75a" },{ 64,"weapon_revolver" },{ 500,"weapon_bayonet" },{ 505,"weapon_knife_flip" },{ 506,"weapon_knife_gut" },
		{ 507,"weapon_knife_karambit" },{ 508,"weapon_knife_m9_bayonet" },{ 509,"weapon_knife_tactical" },{ 512,"weapon_knife_falchion" },{ 514,"weapon_knife_tactical" },{ 515,"weapon_knife_butterfly" },{ 516,"weapon_knife_push" },
		{ 31,"weapon_taser" },{ 47,"weapon_decoy" },{ 55,"item_defuser" },{ 43,"weapon_flashbang" },{ 44,"weapon_hegrenade" },{ 51,"item_helmet" },{ 48,"weapon_incgrenade" },{ 50,"item_kevlar" },{ 46,"weapon_molotov" },{ 45,"weapon_smokegrenade" },{ 49,"c4" }
		};
		for (auto& tmpp : wpns) {
			if (fnv::hash_runtime(s.c_str()) == fnv::hash_runtime(tmpp.second.c_str())) {
				wtex_map[tmpp.first] = std::make_pair(Images[i - 2], Images[i - 1]);
				//cout << tmpp.first << " Initialized" << endl;
			}
		}
	}
	END();
}

CRender::~CRender()
{
	//MUT("CRender::~CRender()");

	if (m_pStateBlockDraw)
		m_pStateBlockDraw = nullptr;

	for (auto& m : font_map) {
		m.second->GetTexture()->Release();
		delete m.second;
		m.second = nullptr;
	}
	font_map.clear();
	for (auto& im : wtex_map) {
		im.second.second->Release();
		delete im.second.second;
		im.second.second = nullptr;
	}
	CursorsTexture->Release();
	delete CursorsTexture;
	m_pDevice = nullptr;
	//END();
}

bool CRender::CreateObject()
{
	MUT("CRender::CreateObject");

	

	HDC hdc = CreateCompatibleDC(NULL);
	auto height = -MulDiv(46, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	DeleteDC(hdc);
	font_map["DroidBold"] = new PizdaFont("Droid Sans", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_BOLD, m_pDevice);//new PizdaFont(CreateFont(100, 50, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
	font_map["Calibri"] = new PizdaFont("Calibri", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_BOLD, m_pDevice);
	font_map["undefeated"] = new PizdaFont("undefeated", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_BOLD, m_pDevice);
																												//OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		//DEFAULT_PITCH | FF_DONTCARE, "Droid Sans"), 100, 0xFFFFFF, m_pDevice);
	font_map["Droid"] = new PizdaFont("Droid Sans", 46.f, 0xFFFFFF);//new PizdaFont(CreateFont(100, 50, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
		//OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		//DEFAULT_PITCH | FF_DONTCARE, "Droid Sans"), 100, 0xFFFFFF, m_pDevice);
	font_map["BigNoodleBold"] = new PizdaFont("BigNoodleTitlingCyr", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_BOLD);
	font_map["BigNoodle"] = new PizdaFont("BigNoodleTitlingCyr", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_NONE);//new PizdaFont(CreateFont(100, 50, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
		//OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		//DEFAULT_PITCH | FF_DONTCARE, "BigNoodleTitlingCyr"), 100, 0xFFFFFF, m_pDevice);
	font_map["DeusExBold"] = new PizdaFont("DeusEx", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_BOLD);//new PizdaFont(CreateFont(100, 50, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
		//OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		//DEFAULT_PITCH | FF_DONTCARE, "DeusEx"), 100, 0xFFFFFF, m_pDevice);
	font_map["DeusEx"] = new PizdaFont("DeusEx", 46.f, 0xFFFFFF, PizdaFont::Flags::FLAG_NONE);//new PizdaFont(CreateFont(100, 50, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
		//OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		//DEFAULT_PITCH | FF_DONTCARE, "DeusEx"), 100, 0xFFFFFF, m_pDevice);
	if (D3D_OK == m_pDevice->CreateTexture(90, 27, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &CursorsTexture, NULL)) {
		D3DLOCKED_RECT LockedRect;
		if (D3D_OK == CursorsTexture->LockRect(0, &LockedRect, NULL, 0))
		{
			static const char CURSORS[90 * 27 + 1] =
			{
				"..-         -XXX XXX-    X    -           X           -XXXXXXX          -          XXXXXXX"
				"..-         -X..X..X-   X.X   -          X.X          -X.....X          -          X.....X"
				"---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X"
				"X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X"
				"XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X"
				"X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X"
				"X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX"
				"X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      "
				"X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       "
				"X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        "
				"X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         "
				"X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          "
				"X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           "
				"X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            "
				"X..........X-X..X..X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           "
				"X......XXXXX-XXX XXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          "
				"X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          "
				"X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       ------------------------------------"
				"X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           "
				"XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           "
				"      X..X          -  X...X  -         X...X         -  X..X           X..X  -           "
				"       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           "
				"------------        -    X    -           X           -X.....................X-           "
				"                    ----------------------------------- X...XXXXXXXXXXXXX...X -           "
				"                                                      -  X..X           X..X  -           "
				"                                                      -   X.X           X.X   -           "
				"                                                      -    XX           XX    -           "
			};

			for (auto i = 0; i < 8; i++) {
				MiscUvs[i] = UVTable(CURSOR_COORDINATES[i][0].x / 90.f, 1.f - (CURSOR_COORDINATES[i][0].y + CURSOR_COORDINATES[i][1].y) / 27.f, (CURSOR_COORDINATES[i][0].x + CURSOR_COORDINATES[i][1].x) / 90.f, 1.f - (CURSOR_COORDINATES[i][0].y / 27.f));
			}
			PDWORD DestData = (PDWORD)LockedRect.pBits;
			for (int y = 0, n = 0; y < 27; y++) {
				DWORD arr[90];
				for (int x = 0; x < 90; x++, n++) {
					const int offset0 = x;// +y * 90;
					arr[offset0] = CURSORS[n] == '.' ? 0xFFFFFFFF : (CURSORS[n] == 'X' ? 0xFF000000 : 0x00000000);
				}
				memcpy(DestData, arr, 90 * sizeof(DWORD));
				DestData += (LockedRect.Pitch / 4);
			}
			CursorsTexture->UnlockRect(0);
		}
	}
	/*
	D3DCAPS9 caps;

	int major_vs, minor_vs;
	int major_ps, minor_ps;
	m_pDevice->GetDeviceCaps(&caps);

	DWORD VERSPX1 = D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion);
	DWORD VERSPX2 = D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion);

	char szVersion[32];
	sprintf(szVersion, "ps_%d_%d", VERSPX1, VERSPX2);

	char szShader[256];
	sprintf(szShader, "%s \n mov oC0, c0", szVersion);

	ID3DXBuffer* pShaderBuf = NULL;

	D3DXAssembleShader(szShader, sizeof(szShader), NULL, NULL, 0, &pShaderBuf, NULL);
	if (pShaderBuf)
	m_pDevice->CreatePixelShader((const DWORD*)pShaderBuf->GetBufferPointer(), &pixelshader);

	m_pDevice->GetRenderTarget(0, &backbuffer);
	backbuffer->GetDesc(&desc);
	backbuffer->Release();

	m_pDevice->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &sil_texture, NULL);
	sil_texture->GetSurfaceLevel(0, &ppSurfaceLevel);

	m_pDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX),
	0,
	D3DFVF_XYZRHW | D3DFVF_TEX1,
	D3DPOOL_MANAGED,
	&v_buffer,
	NULL);

	*/
	return true;
	END();
}

void CRender::OnLostDevice()
{
	//MUT("CRender::OnLostDevice()");
	if (!m_pDevice)
		return;

	if (m_pStateBlockDraw)
		m_pStateBlockDraw->Release();
	m_pStateBlockDraw = nullptr;

	if (this->_IndexBuffer)
		this->_IndexBuffer->Release();
	this->_IndexBuffer = nullptr;

	if (this->_VertexBuffer)
		this->_VertexBuffer->Release();
	this->_VertexBuffer = nullptr;

	//END();
}

void CRender::OnResetDevice()
{
}

void CRender::BeginRender()
{
	m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlockDraw);

	if (m_pStateBlockDraw)
		m_pStateBlockDraw->Capture();

	m_pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &dwOld_D3DRS_COLORWRITEENABLE);
	m_pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &dwOld_D3DRS_SRGBWRITEENABLE);
	m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	m_pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

	m_pDevice->GetTransform(D3DTS_WORLD, &last_world);
	m_pDevice->GetTransform(D3DTS_VIEW, &last_view);
	m_pDevice->GetTransform(D3DTS_PROJECTION, &last_projection);

	int iScreenWidth, iScreenHeight;
	g_EngineClient->GetScreenSize(iScreenWidth, iScreenHeight);
	D3DVIEWPORT9 vp;
	vp.X = vp.Y = 0;
	vp.Width = (DWORD)iScreenWidth;
	vp.Height = (DWORD)iScreenHeight;
	vp.MinZ = 0.0f;
	vp.MaxZ = 1.0f;
	m_pDevice->SetViewport(&vp);

	float L = 0.f;
	float R = iScreenWidth;
	float T = 0.f;
	float B = iScreenHeight;
	D3DMATRIX mat_identity = { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } };
	D3DMATRIX mat_projection =
	{
		2.0f / (R - L),   0.0f,         0.0f,  0.0f,
		0.0f,         2.0f / (T - B),   0.0f,  0.0f,
		0.0f,         0.0f,         0.5f,  0.0f,
		(L + R) / (L - R),  (T + B) / (B - T),  0.5f,  1.0f,
	};
	m_pDevice->SetTransform(D3DTS_WORLD, &mat_identity);
	m_pDevice->SetTransform(D3DTS_VIEW, &mat_identity);
	m_pDevice->SetTransform(D3DTS_PROJECTION, &mat_projection);

}

void CRender::EndRender()
{
	m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, dwOld_D3DRS_COLORWRITEENABLE);
	m_pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, dwOld_D3DRS_SRGBWRITEENABLE);
	m_pDevice->SetTransform(D3DTS_WORLD, &last_world);
	m_pDevice->SetTransform(D3DTS_VIEW, &last_view);
	m_pDevice->SetTransform(D3DTS_PROJECTION, &last_projection);

	if (m_pStateBlockDraw)
	{
		m_pStateBlockDraw->Apply();
		m_pStateBlockDraw->Release();
	}

}

void CRender::SetVertexState()
{
	m_pDevice->SetTexture(NULL, NULL);
	m_pDevice->SetPixelShader(NULL);

	m_pDevice->SetVertexShader(NULL);

	m_pDevice->SetFVF(D3DFVF_CUSTOM_VERTEX);

	m_pDevice->SetRenderState(D3DRS_LIGHTING, false);
	m_pDevice->SetRenderState(D3DRS_FOGENABLE, false);

	m_pDevice->SetRenderState(D3DRS_ZENABLE, false);
	m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	m_pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	m_pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	m_pDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
	m_pDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);


	m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
	m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

	m_pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);

	m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

}
#pragma endregion
