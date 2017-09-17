#define FROM 3564
#define TO 3925

void chain() {
	TChain *ch = new TChain("h101");
	for (int i = FROM; i <= TO; i++) {
		char *filename = new char[255];
		sprintf(filename, "../data/beam_%04d.root", i);
		ch->Add(filename);
	}
}
