#ifndef	__CONVERTUTF_H__
#define	__CONVERTUTF_H__

namespace Unicode
{

	typedef unsigned long	UTF32;	/* at least 32 bits */
	typedef wchar_t	UTF16;	/* at least 16 bits */
	typedef unsigned char	UTF8;	/* typically 8 bits */

	enum ConversionResult
	{
		ConversionOK, 		/* conversion successful */
		SourceExhausted,	/* partial character in source, but hit end */
		SourceIllegal		/* source sequence is illegal/malformed */
	};

	/// <summary>
	///   Converts from UTF-16 to UTF-8.
	/// </summary>
	void Convert( const std::wstring& src, std::string& dest );
	std::string Convert( const std::wstring& src );

	/// <summary>
	///   Converts from UTF-16 to UTF-8.
	/// </summary>
	void Convert( const std::string& src, std::wstring& dest );
	std::wstring Convert( const std::string& src );
}

namespace Exception
{
	template< typename ResultType >
	class UTFConversion : public std::runtime_error
	{
	public:
		const ResultType PartialResult;

		virtual ~UTFConversion() throw() {}
		UTFConversion( const ResultType& result, const std::string& msg ) :
			runtime_error( msg ),
			PartialResult( result ) {}
	};
}

#endif // __CONVERTUTF_H__
