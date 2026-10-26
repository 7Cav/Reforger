//! The GM browser search only matches localised names: WidgetManager.SearchLocalized looks up each
//! name as a string table ID, so an entry named in plain text (our command trucks, the AFRF SOF groups,
//! most mod content) never comes up. Let the original filter run without the search to get the
//! label-filtered list, then search that list with both the localised match and a plain text match.
modded class SCR_ContentBrowserEditorComponent
{
	override void FilterEntries()
	{
		string search = m_sCurrentSearchText;
		if (search.IsEmpty() || GetExtendedEntity())
		{
			super.FilterEntries();
			return;
		}

		m_sCurrentSearchText = string.Empty;
		super.FilterEntries();
		m_sCurrentSearchText = search;

		int count = m_aLocalizationKeys.Count();
		array<int> localisedHits = {};
		WidgetManager.SearchLocalized(search, m_aLocalizationKeys, localisedHits);

		array<bool> isLocalisedHit = {};
		isLocalisedHit.Resize(count);
		foreach (int hit : localisedHits)
		{
			isLocalisedHit[hit] = true;
		}

		string needle = search;
		needle.ToLower();

		array<int> matched = {};
		string name;
		for (int i = 0; i < count; i++)
		{
			name = m_aLocalizationKeys[i];
			if (isLocalisedHit[i])
			{
				matched.Insert(m_aFilteredPrefabIDs[i]);
				continue;
			}

			if (name.StartsWith("#"))
				continue;

			name.ToLower();
			if (name.Contains(needle))
				matched.Insert(m_aFilteredPrefabIDs[i]);
		}

		m_aFilteredPrefabIDs.Copy(matched);
		m_iFilteredPrefabIDsCount = m_aFilteredPrefabIDs.Count();
		Event_OnBrowserEntriesFiltered.Invoke();
	}
}
