//Header file to define IPT Class type IPTEntry
enum pageTypeFormat {CODE, DATA, UNINIT, STACK,MIXED};
enum pageLocationFormat {MAINMEMORY, SWAPFILE, EXECUTABLE, OTHER};

class IPTEntry {
	public:
	int virtualPage; 
	int physicalPage;
	pageTypeFormat pageType;
	pageLocationFormat pageLocation;
	bool valid;
	bool dirty;
	bool use;
	bool readOnly;
	int PID;
	int test;
};
