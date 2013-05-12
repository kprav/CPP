
class NewTranslationEntry {
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
	int position;
};
