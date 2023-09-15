DWORD Rect(LPJASS j) {
    API_ALLOC(BOX2, rect);
    rect->min.x = jass_checknumber(j, 1);
    rect->min.y = jass_checknumber(j, 2);
    rect->max.x = jass_checknumber(j, 3);
    rect->max.y = jass_checknumber(j, 4);
    return 1;
}
DWORD RectFromLoc(LPJASS j) {
    LPCVECTOR2 min = jass_checkhandle(j, 1, "location");
    LPCVECTOR2 max = jass_checkhandle(j, 2, "location");
    API_ALLOC(BOX2, rect);
    rect->min = *min;
    rect->max = *max;
    return 1;
}
DWORD RemoveRect(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return 0;
}
DWORD SetRect(LPJASS j) {
    LPBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    whichRect->min.x = jass_checknumber(j, 2);
    whichRect->min.y = jass_checknumber(j, 3);
    whichRect->max.x = jass_checknumber(j, 4);
    whichRect->max.y = jass_checknumber(j, 5);
    return 0;
}
DWORD SetRectFromLoc(LPJASS j) {
    LPBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    LPCVECTOR2 min = jass_checkhandle(j, 2, "location");
    LPCVECTOR2 max = jass_checkhandle(j, 3, "location");
    whichRect->min = *min;
    whichRect->max = *max;
    return 0;
}
DWORD MoveRectTo(LPJASS j) {
    LPBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    VECTOR2 newCenterLoc = {
        jass_checknumber(j, 2),
        jass_checknumber(j, 3),
    };
    Box2_moveTo(whichRect, &newCenterLoc);
    return 0;
}
DWORD MoveRectToLoc(LPJASS j) {
    LPBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    LPCVECTOR2 newCenterLoc = jass_checkhandle(j, 2, "location");
    Box2_moveTo(whichRect, newCenterLoc);
    return 0;
}
DWORD GetRectCenterX(LPJASS j) {
    LPCBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, Box2_center(whichRect).x);
}
DWORD GetRectCenterY(LPJASS j) {
    LPCBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, Box2_center(whichRect).y);
}
DWORD GetRectMinX(LPJASS j) {
    LPCBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, whichRect->min.x);
}
DWORD GetRectMinY(LPJASS j) {
    LPCBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, whichRect->min.y);
}
DWORD GetRectMaxX(LPJASS j) {
    LPCBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, whichRect->max.x);
}
DWORD GetRectMaxY(LPJASS j) {
    LPCBOX2 whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, whichRect->max.y);
}
DWORD CreateRegion(LPJASS j) {
    API_ALLOC(REGION, region);
    return 1;
}
DWORD RemoveRegion(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    return 0;
}
DWORD RegionAddRect(LPJASS j) {
    LPREGION whichRegion = jass_checkhandle(j, 1, "region");
    LPBOX2 r = jass_checkhandle(j, 2, "rect");
    whichRegion->rects[whichRegion->num_rects++] = *r;
    return 0;
}
DWORD RegionClearRect(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE r = jass_checkhandle(j, 2, "rect");
    return 0;
}
DWORD RegionAddCell(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD RegionAddCellAtLoc(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD RegionClearCell(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD RegionClearCellAtLoc(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD Location(LPJASS j) {
    API_ALLOC(VECTOR2, location);
    location->x = jass_checknumber(j, 1);
    location->y = jass_checknumber(j, 2);
    return 1;
}
DWORD RemoveLocation(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    return 0;
}
DWORD MoveLocation(LPJASS j) {
    LPVECTOR2 whichLocation = jass_checkhandle(j, 1, "location");
    whichLocation->x = jass_checknumber(j, 2);
    whichLocation->y = jass_checknumber(j, 3);
    return 0;
}
DWORD GetLocationX(LPJASS j) {
    LPVECTOR2 whichLocation = jass_checkhandle(j, 1, "location");
    return jass_pushnumber(j, whichLocation->x);
}
DWORD GetLocationY(LPJASS j) {
    LPVECTOR2 whichLocation = jass_checkhandle(j, 1, "location");
    return jass_pushnumber(j, whichLocation->y);
}
