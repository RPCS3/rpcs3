#ifndef RasterFont_Header
#define RasterFont_Header

class RasterFont {
protected:
	int	fontOffset;
	
public:
	RasterFont();
	~RasterFont(void);
	static int debug;
	
	// some useful constants
	enum	{char_width = 10};
	enum	{char_height = 15};
	
	// and the happy helper functions
	void printString(const char *s, double x, double y, double z=0.0);
	void printCenteredString(const char *s, double y, int screen_width, double z=0.0);
};

#endif
