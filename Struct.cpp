struct BlockCoords {
	int xBlockCoord;
	int yBlockCoord;
};

BlockCoords* AddStruct(BlockCoords* Obj, const int Count);
void setData(BlockCoords* Obj, const int Count);
void getData(const BlockCoords* Obj, const int Count);

BlockCoords* Blocks = 0;
int BlocksCount = 0;