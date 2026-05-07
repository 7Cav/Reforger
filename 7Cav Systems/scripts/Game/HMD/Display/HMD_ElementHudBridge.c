//------------------------------------------------------------------------------------------------
//! HMD pool mirror HUD: flat `rootFrame` + `MarkerDot`/`MarkerLabel` pool (32 slots),
//! `FillMarkerPoolFromRoot`-style init (vanilla circle seed), runtime `FrameSlot.SetPos`/`SetSize` on dots.
class HMD_ElementHudBridge
{
	protected static const int ENTRY_POOL = 32;

	//! Laser `HUDMarkerDisplayHelper.MARKER_DOT_TEXTURE` — seed pool so `ImageWidget` matches proven HUD path.
	protected static const string HMD_HUD_TEX_POOL_SEED = "{73B3D8BBB785B5B9}UI/Textures/Common/circleFull.edds";
	//! Pool mirror icon textures by informational `m_iClassType` / target designation.
	protected static const string HMD_HUD_TEX_INFO_CLASS0 = "{266AE3BC015F539B}Assets/LaserUI/IFF_Marker.edds";
	protected static const string HMD_HUD_TEX_INFO_CLASS1 = "{7D8473979D08418B}Assets/LaserUI/Waypoint_Marker.edds";
	protected static const string HMD_HUD_TEX_INFO_CLASS2 = "{20A852AA979409C4}Assets/LaserUI/Reference_Marker.edds";
	protected static const string HMD_HUD_TEX_LASE_ELEMENT = "{59E06B62680B6CB3}Assets/Radar/Lase_Element1.edds";

	protected static const float MARKER_DOT_SIZE = 32.0;
	//! Gap (px) between dot bottom edge and label top (`MarkerLabel` sits under the icon, centered on world screen X).
	protected static const float LABEL_OFFSET_Y = 4.0;
	//! Match `MarkerLabel*` slot SizeX / SizeY in `HMD_ElementOverlay.layout`.
	protected static const float LABEL_W = 344.0;
	protected static const float LABEL_H = 28.0;
	//! Max HUD strength for **target designation** pool rows (× distance fade).
	protected static const float HMD_ELEMENT_OVERLAY_UI_OPACITY = 0.5;
	//! Max HUD strength for **informational** pool rows (IFF / waypoint / reference / generic markers only).
	protected static const float HMD_MARKER_INFORMATIONAL_UI_OPACITY = 0.85;

	protected static bool s_bInited;
	protected static Widget s_wRoot;
	protected static ref array<TextWidget> s_aEntries = {};
	protected static ref array<ImageWidget> s_aIcons = {};
	protected static ref array<string> s_aLastIconTexPath = {};

	//------------------------------------------------------------------------------------------------
	protected static float HmdOverlayOpacity(float fadeOpacity, float uiOpacityCap)
	{
		float v = fadeOpacity * uiOpacityCap;
		if (v > 1.0)
			v = 1.0;
		return v;
	}

	//------------------------------------------------------------------------------------------------
	//! Dot + label share `m_iHudColorArgb` (`Color.PackToInt()` / `Color.FromInt()`).
	protected static Color HmdPoolMirrorRowHudColor(HMD_PoolMirrorRow row)
	{
		if (!row)
			return Color.FromRGBA(255, 255, 255, 255);
		return Color.FromInt(row.m_iHudColorArgb);
	}

	//------------------------------------------------------------------------------------------------
	protected static string HmdPoolRowIconTexturePath(HMD_PoolMirrorRow row)
	{
		if (!row)
			return HMD_HUD_TEX_INFO_CLASS0;
		if (row.m_eKind == EHmdElementKind.TARGET_DESIGNATION)
			return HMD_HUD_TEX_LASE_ELEMENT;
		if (row.m_eKind == EHmdElementKind.INFORMATIONAL)
		{
			if (row.m_iClassType == 0)
				return HMD_HUD_TEX_INFO_CLASS0;
			if (row.m_iClassType == 1)
				return HMD_HUD_TEX_INFO_CLASS1;
			if (row.m_iClassType == 2)
				return HMD_HUD_TEX_INFO_CLASS2;
			if (row.m_iClassType == 3)
				return HMD_HUD_TEX_LASE_ELEMENT;
			return HMD_HUD_TEX_INFO_CLASS0;
		}
		return HMD_HUD_TEX_INFO_CLASS0;
	}

	//------------------------------------------------------------------------------------------------
	//! API: `LoadImageTexture(slot, resource, noCache, fromLocalStorage)` — booleans are not “transparency modes”.
	protected static bool HmdTryLoadImageTextureSlot0(ImageWidget dot, string texPath)
	{
		if (!dot || texPath == string.Empty)
			return false;
		return dot.LoadImageTexture(0, texPath, false, false);
	}

	//! Per UI API: alpha-blended rendering for `ImageWidget` (marker RGBA / soft edges).
	protected static void HmdConfigureMarkerImageBlend(ImageWidget img)
	{
		if (!img)
			return;
		img.SetFlags(img.GetFlags() | WidgetFlags.BLEND);
	}

	//------------------------------------------------------------------------------------------------
	//! Match readout font size; no outline/shadow (flat white text).
	protected static void HmdApplyReadoutTypographyToPoolLabel(TextWidget tw)
	{
		if (!tw)
			return;
		tw.SetExactFontSize(25);
	}

	//------------------------------------------------------------------------------------------------
	protected static void HmdApplyRowIconTexture(ImageWidget dot, int slotIndex, string texPath)
	{
		if (!dot || texPath == string.Empty)
			return;
		while (s_aLastIconTexPath.Count() <= slotIndex)
			s_aLastIconTexPath.Insert(string.Empty);
		if (texPath != s_aLastIconTexPath[slotIndex])
		{
			if (HmdTryLoadImageTextureSlot0(dot, texPath))
			{
				dot.SetImage(0);
				HmdConfigureMarkerImageBlend(dot);
				s_aLastIconTexPath[slotIndex] = texPath;
			}
			else
				HMD_HmdDebug.CliEligibility(string.Format("HMD_ElementHudBridge: LoadImageTexture failed for %1", texPath));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Laser `HUDMarkerDisplayHelper.FillMarkerPoolFromRoot` — resolve `MarkerDot` / `MarkerLabel` and seed slot 0.
	protected static void HmdFillElementPoolFromRoot(Widget root, int poolSize, array<ImageWidget> outDots, array<TextWidget> outLabels)
	{
		if (!root || !outDots || !outLabels || poolSize <= 0)
			return;
		for (int i = 0; i < poolSize; i++)
		{
			Widget wDot = root.FindAnyWidget(string.Format("MarkerDot%1", i));
			if (wDot)
			{
				ImageWidget img = ImageWidget.Cast(wDot);
				if (img)
				{
					img.LoadImageTexture(0, HMD_HUD_TEX_POOL_SEED, false, false);
					HmdConfigureMarkerImageBlend(img);
					outDots.Insert(img);
					img.SetVisible(false);
				}
			}
			Widget wLbl = root.FindAnyWidget(string.Format("MarkerLabel%1", i));
			if (wLbl)
			{
				TextWidget txt = TextWidget.Cast(wLbl);
				if (txt)
				{
					HmdApplyReadoutTypographyToPoolLabel(txt);
					outLabels.Insert(txt);
					txt.SetVisible(false);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	static void ResetForNewPlaySession()
	{
		s_bInited = false;
		s_wRoot = null;
		s_aEntries.Clear();
		s_aIcons.Clear();
		s_aLastIconTexPath.Clear();
	}

	//------------------------------------------------------------------------------------------------
	static void InitOnce()
	{
		if (s_bInited)
			return;
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.GetHUDManager();
		if (!hud)
			return;
		s_wRoot = hud.CreateLayout("{6900000200000001}UI/layouts/HUD/HMD_ElementOverlay.layout", EHudLayers.MEDIUM, 0);
		if (!s_wRoot)
			return;
		s_bInited = true;
		s_aEntries.Clear();
		s_aIcons.Clear();
		s_aLastIconTexPath.Clear();
		HmdFillElementPoolFromRoot(s_wRoot, ENTRY_POOL, s_aIcons, s_aEntries);
		for (int k = 0; k < s_aIcons.Count(); k++)
			s_aLastIconTexPath.Insert(HMD_HUD_TEX_POOL_SEED);
	}

	//------------------------------------------------------------------------------------------------
	static void Update(float timeSlice)
	{
		if (!s_wRoot)
			return;
		Game game = GetGame();
		if (!game)
			return;
		World world = game.GetWorld();
		WorkspaceWidget ws = game.GetWorkspace();
		ChimeraGame chGame = ChimeraGame.Cast(game);
		if (!world || !ws || !chGame)
			return;
		if (!HMD_ElementVisualizationRegistry.AnyGateActive())
		{
			s_wRoot.SetVisible(false);
			return;
		}
		HMD_GlobalHmdElementPoolComponent pool = HMD_GlobalHmdElementPoolComponent.FindPool();
		if (!pool)
		{
			s_wRoot.SetVisible(false);
			return;
		}
		s_wRoot.SetVisible(true);
		vector camTM[4];
		CameraManager mgr = chGame.GetCameraManager();
		CameraBase cam;
		if (mgr)
			cam = mgr.CurrentCamera();
		if (!cam)
			return;
		cam.GetTransform(camTM);
		vector camPos = camTM[3];
		float maxD = HMD_ElementVisualizationRegistry.GetEffectiveMaxViewDistanceM();
		array<ref HMD_PoolMirrorRow> rows = new array<ref HMD_PoolMirrorRow>();
		pool.GetMirrorRows(rows);
		int slot = 0;
		float dSize = MARKER_DOT_SIZE;
		for (int r = 0; r < rows.Count(); r++)
		{
			if (slot >= s_aEntries.Count() || slot >= s_aIcons.Count())
				break;
			HMD_PoolMirrorRow row = rows[r];
			if (!row)
				continue;
			if (row.m_eKind == EHmdElementKind.INFORMATIONAL)
			{
				if (!HMD_ElementVisualizationInput.ShowInformationalMirrorRow(row))
					continue;
				RplId turretScope = HMD_ElementVisualizationRegistry.GetActiveTurretInformationalScopeParent0();
				if (turretScope != RplId.Invalid() && (row.m_iClassType == 1 || row.m_iClassType == 2) && row.m_Parent0 != turretScope)
				{
					HMD_HmdDebug.CliLaserRefHudThrottled(string.Format("Skip WP/RP row (turret scope): text=%1", row.m_sText), 4000);
					continue;
				}
			}
			if (row.m_eKind == EHmdElementKind.TARGET_DESIGNATION)
			{
				bool showDes = HMD_ElementVisualizationInput.ShowTargetDesignations();
				if (!showDes && row.m_bShowEvenWhenDesignationHudOff && HMD_ElementVisualizationInput.LocalVehicleLaserMarkingActiveForOwnLaserHudBypass())
					showDes = true;
				if (!showDes)
					continue;
			}
			vector hudWorldPos = row.m_vPositionWorld;
			vector localDesHit;
			if (row.m_eKind == EHmdElementKind.TARGET_DESIGNATION
				&& HMD_LaserMarkingCoreComponent.StaticTryGetLocalHudDesignationWorldHitForRow(row, localDesHit))
				hudWorldPos = localDesHit;
			float dist = (camPos - hudWorldPos).Length();
			float opacity = HMD_ElementVisualFadeUtils.OpacityFromDistanceM(dist, maxD);
			TextWidget tw = s_aEntries[slot];
			ImageWidget iw = s_aIcons[slot];
			if (!tw || !iw)
				continue;
			if (opacity <= 0.01)
			{
				iw.SetVisible(false);
				tw.SetVisible(false);
				slot++;
				continue;
			}
			//! No view culling: show pool markers even when off-screen or behind the camera plane (legacy HUD hid those).
			vector screen = ws.ProjWorldToScreen(hudWorldPos, world);
			float posX = screen[0];
			float posY = screen[1];
			float dotLeft = posX - (dSize * 0.5);
			float dotTop = posY - (dSize * 0.5);
			FrameSlot.SetPos(iw, dotLeft, dotTop);
			FrameSlot.SetSize(iw, dSize, dSize);
			//! Label under dot; world point stays dot center. `LABEL_W` strip centered on `posX`.
			float labelX = posX - (LABEL_W * 0.5);
			float labelY = dotTop + dSize + LABEL_OFFSET_Y;
			FrameSlot.SetPos(tw, labelX, labelY);
			FrameSlot.SetSize(tw, LABEL_W, LABEL_H);
			HmdApplyRowIconTexture(iw, slot, HmdPoolRowIconTexturePath(row));
			float uiCap = HMD_ELEMENT_OVERLAY_UI_OPACITY;
			if (row.m_eKind == EHmdElementKind.INFORMATIONAL)
				uiCap = HMD_MARKER_INFORMATIONAL_UI_OPACITY;
			float hudOpacity = HmdOverlayOpacity(opacity, uiCap);
			Color hudCol = HmdPoolMirrorRowHudColor(row);
			iw.SetColor(hudCol);
			tw.SetColor(hudCol);
			iw.SetOpacity(hudOpacity);
			tw.SetOpacity(hudOpacity);
			string line;
			if (row.m_eKind == EHmdElementKind.INFORMATIONAL)
			{
				//! Informational pool rows (IFF / WP / RP / generic): label text only — never show `m_iCode`.
				line = row.m_sText;
			}
			else if (row.m_eKind == EHmdElementKind.TARGET_DESIGNATION)
			{
				//! Class 3: IFF-style designations — tag only (same as informational), not laser code.
				if (row.m_iClassType == 3)
					line = row.m_sText;
				else if (row.m_iCode > 0)
					line = string.Format("%1", row.m_iCode);
				else
					line = "----";
			}
			else
			{
				line = row.m_sText;
			}
			tw.SetText(line);
			iw.SetVisible(true);
			tw.SetVisible(true);
			slot++;
		}
		for (int j = slot; j < s_aEntries.Count(); j++)
		{
			if (j < s_aIcons.Count() && s_aIcons[j])
				s_aIcons[j].SetVisible(false);
			s_aEntries[j].SetVisible(false);
			if (j < s_aLastIconTexPath.Count())
				s_aLastIconTexPath[j] = string.Empty;
		}

		HMD_HmdDebug.CliHudDrawSummaryThrottled(string.Format("registryGate=%1 pool=%2 mirrorRows=%3 overlaySlotsUsed=%4 showIff=%5 showNonIffInfo=%6 showDes=%7", HMD_ElementVisualizationRegistry.AnyGateActive(), pool != null, rows.Count(), slot, HMD_ElementVisualizationInput.ShowIffInformationalMarkers(), HMD_ElementVisualizationInput.ShowNonIffInformationalMarkers(), HMD_ElementVisualizationInput.ShowTargetDesignations()), 2500);
	}
}
