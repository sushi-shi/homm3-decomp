/* All TIL local types of the NH3API v2.1 IDB, rendered by python-idb. HD-pressing reference. */

enum __TI_flags
{
  TI_IsConst = 0x1,
  TI_IsVolatile = 0x2,
  TI_IsUnaligned = 0x4,
  TI_IsPure = 0x8,
  TI_IsWinRT = 0x10,
};

enum __CT_flags
{
  CT_IsSimpleType = 0x1,
  CT_ByReferenceOnly = 0x2,
  CT_HasVirtualBase = 0x4,
  CT_IsWinRTHandle = 0x8,
  CT_IsStdBadAlloc = 0x10,
};

struct _SCOPETABLE_ENTRY
{
  int EnclosingLevel;
  void* FilterFunc;
  void* HandlerFunc;
};

unsigned int;

_SCOPETABLE_ENTRY*;

struct _EH3_EXCEPTION_REGISTRATION
{
  _EH3_EXCEPTION_REGISTRATION* Next;
  PVOID ExceptionHandler;
  PSCOPETABLE_ENTRY ScopeTable;
  DWORD TryLevel;
};

struct _EH3_EXCEPTION_REGISTRATION { };

_EH3_EXCEPTION_REGISTRATION*;

struct CPPEH_RECORD
{
  DWORD old_esp;
  EXCEPTION_POINTERS* exc_ptr;
  _EH3_EXCEPTION_REGISTRATION registration;
};

struct tagPOINT
{
  LONG x;
  LONG y;
};

int;

struct tagRECT
{
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
};

typedef WNDCLASSA tagWNDCLASSA;

struct tagWNDCLASSA
{
  UINT style;
  WNDPROC lpfnWndProc;
  int cbClsExtra;
  int cbWndExtra;
  HINSTANCE hInstance;
  HICON hIcon;
  HCURSOR hCursor;
  HBRUSH hbrBackground;
  LPCSTR lpszMenuName;
  LPCSTR lpszClassName;
};

unsigned int;

LRESULT (__stdcall *WNDPROC)(HWND, UINT, WPARAM, LPARAM);

HWND__*;

struct HWND__
{
  int unused;
};

typedef WPARAM UINT_PTR;

unsigned int;

typedef LPARAM LONG_PTR;

int;

typedef LRESULT LONG_PTR;

HINSTANCE__*;

struct HINSTANCE__
{
  int unused;
};

HICON__*;

struct HICON__
{
  int unused;
};

typedef HCURSOR HICON;

HBRUSH__*;

struct HBRUSH__
{
  int unused;
};

CHAR*;

int8;

struct tagMSG
{
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
};

typedef POINT tagPOINT;

typedef RECT tagRECT;

struct WSAData
{
  WORD wVersion;
  WORD wHighVersion;
  int8[257] szDescription;
  int8[129] szSystemStatus;
  unsigned int16 iMaxSockets;
  unsigned int16 iMaxUdpDg;
  int8* lpVendorInfo;
};

unsigned int16;

struct sockaddr
{
  ADDRESS_FAMILY sa_family;
  CHAR[14] sa_data;
};

typedef ADDRESS_FAMILY USHORT;

unsigned int16;

struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
};

struct _SYSTEMTIME
{
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
};

struct tagPAINTSTRUCT
{
  HDC hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE[32] rgbReserved;
};

HDC__*;

struct HDC__
{
  int unused;
};

int;

unsigned int8;

struct in_addr
{
  in_addr::$D689D43D03D53F61DA021DB261182132 S_un;
};

union in_addr::$D689D43D03D53F61DA021DB261182132
{
  in_addr::$D689D43D03D53F61DA021DB261182132::$F085A1F6735C7CEA9C650424FAF692B1 S_un_b;
  in_addr::$D689D43D03D53F61DA021DB261182132::$B9D7529FFD1842B2B059BD2E926FB2C5 S_un_w;
  ULONG S_addr;
};

struct in_addr::$D689D43D03D53F61DA021DB261182132::$F085A1F6735C7CEA9C650424FAF692B1
{
  UCHAR s_b1;
  UCHAR s_b2;
  UCHAR s_b3;
  UCHAR s_b4;
};

unsigned int8;

struct in_addr::$D689D43D03D53F61DA021DB261182132::$B9D7529FFD1842B2B059BD2E926FB2C5
{
  USHORT s_w1;
  USHORT s_w2;
};

unsigned int;

struct _OFSTRUCT
{
  BYTE cBytes;
  BYTE fFixedDisk;
  WORD nErrCode;
  WORD Reserved1;
  WORD Reserved2;
  CHAR[128] szPathName;
};

struct _RTL_CRITICAL_SECTION
{
  PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
  LONG LockCount;
  LONG RecursionCount;
  HSAMPLE OwningThread;
  HSAMPLE LockSemaphore;
  ULONG_PTR SpinCount;
};

_RTL_CRITICAL_SECTION_DEBUG*;

struct _RTL_CRITICAL_SECTION_DEBUG
{
  WORD Type;
  WORD CreatorBackTraceIndex;
  _RTL_CRITICAL_SECTION* CriticalSection;
  LIST_ENTRY ProcessLocksList;
  DWORD EntryCount;
  DWORD ContentionCount;
  DWORD Flags;
  WORD CreatorBackTraceIndexHigh;
  WORD SpareWORD;
};

typedef LIST_ENTRY _LIST_ENTRY;

struct _LIST_ENTRY
{
  _LIST_ENTRY* Flink;
  _LIST_ENTRY* Blink;
};

_SAMPLE*;

unsigned int;

struct _WIN32_FIND_DATAA
{
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  DWORD dwReserved0;
  DWORD dwReserved1;
  CHAR[260] cFileName;
  CHAR[14] cAlternateFileName;
};

typedef FILETIME _FILETIME;

struct _TIME_ZONE_INFORMATION
{
  LONG Bias;
  WCHAR[32] StandardName;
  SYSTEMTIME StandardDate;
  LONG StandardBias;
  WCHAR[32] DaylightName;
  SYSTEMTIME DaylightDate;
  LONG DaylightBias;
};

typedef WCHAR wchar_t;

unsigned int16;

typedef SYSTEMTIME _SYSTEMTIME;

struct _SECURITY_ATTRIBUTES
{
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
};

void*;

struct _STARTUPINFOA
{
  DWORD cb;
  LPSTR lpReserved;
  LPSTR lpDesktop;
  LPSTR lpTitle;
  DWORD dwX;
  DWORD dwY;
  DWORD dwXSize;
  DWORD dwYSize;
  DWORD dwXCountChars;
  DWORD dwYCountChars;
  DWORD dwFillAttribute;
  DWORD dwFlags;
  WORD wShowWindow;
  WORD cbReserved2;
  LPBYTE lpReserved2;
  HSAMPLE hStdInput;
  HSAMPLE hStdOutput;
  HSAMPLE hStdError;
};

CHAR*;

BYTE*;

struct _cpinfo
{
  UINT MaxCharSize;
  BYTE[2] DefaultChar;
  BYTE[12] LeadByte;
};

struct _OSVERSIONINFOA
{
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR[128] szCSDVersion;
};

struct FuncInfoV1
{
  int magicNumber;
  int maxState;
  void* pUnwindMap;
  int nTryBlocks;
  void* pTryBlockMap;
  int nIPMapEntries;
  void* pIPtoStateMap;
};

struct UnwindMapEntry
{
  int toState;
  void* action;
};

struct TryBlockMapEntry
{
  int tryLow;
  int tryHigh;
  int catchHigh;
  int nCatches;
  void* pHandlerArray;
};

struct HandlerType
{
  unsigned int adjectives;
  TypeDescriptor* pType;
  int dispCatchObj;
  void* addressOfHandler;
};

struct TypeDescriptor
{
  unsigned int hash;
  void* spare;
  int8[] name;
};

typedef FILE _iobuf;

struct _iobuf
{
  int8* _ptr;
  int _cnt;
  int8* _base;
  int _flag;
  int _file;
  int _charbuf;
  int _bufsiz;
  int8* _tmpfname;
};

typedef IID GUID;

struct GUID
{
  int Data1;
  ushort Data2;
  ushort Data3;
  uchar[8] Data4;
};

struct _GUID
{
  unsigned int Data1;
  unsigned int16 Data2;
  unsigned int16 Data3;
  unsigned int8[8] Data4;
};

struct std::codecvt_base { };

struct Iostream_init { };

struct unused_87_t { };

struct CImmMouse : CImmDevice
{
  void* m_piApi;
  void* m_piDevice;
};

struct CImmEnclosure : CImmEffect
{
  FEELIT_ENCLOSURE m_enclosure;
  BOOL m_bUseMousePosAtStart;
  CImmEffect* m_pInsideEffect;
};

struct CImmEffect
{
  void* vftable;
  FEELIT_EFFECT m_Effect;
  DWORD[2] m_dwaAxes;
  LONG[2] m_laDirections;
  FEELIT_ENVELOPE m_Envelope;
  GUID m_guidEffect;
  BOOL m_bIsPlaying;
  DWORD m_dwDeviceType;
  void* m_piImmDevice;
  void* m_piImmEffect;
  DWORD m_cAxes;
  DWORD m_dwNoDownload;
  DWORD m_dwIterations;
  int8* m_lpszName;
  BOOL m_bIsInsideEffect;
  CImmEffect* m_pOutsideEffect;
};

struct std::ios_base
{
  void* vftable;
  int _State;
  int _Except;
  int _Fmtfl;
  int _Prec;
  int _Wide;
  void* _Arr;
  void* _Calls;
  void* _Loc;
  size_t _Stdstr;
};

struct std::strstreambuf : std::streambuf
{
  int8* _Pendsave;
  int8* _Seekhigh;
  int _Alsize;
  int _Strmode;
  void* (__cdecl *)(size_t _Pendsave);
  void (__cdecl *)(void* _Pendsave);
};

struct std::_Lockit
{
  int unk;
};

struct std::locale { };

struct std::exception
{
  std::exception::vftable_t* vftable;
  int8* _m_what;
  int _m_doFree;
};

struct std::locale::facet { };

struct unused_97_t { };

struct type_info { };

struct std::locale::_Locimp { };

struct std::_Locinfo { };

struct std::istrstream { };

struct std::ostrstream : std::ostream { };

struct std::ios_base::Init { };

struct std::_Winit { };

struct CSprite : resource
{
  CSequence** s;
  TPalette16* p16;
  TPalette24* p24;
  int numSequences;
  int* validSeqMask;
  int Width;
  int Height;
};

struct TPalette16 : resource
{
  int16[256] Palette;
};

struct TPalette24 : resource
{
  RGB8[256] Palette;
};

struct CSpriteFrame : resource
{
  int DataSize;
  int ImageSize;
  TEncodingMethod EncodingMethod;
  int Width;
  int Height;
  int CroppedWidth;
  int CroppedHeight;
  int CroppedX;
  int CroppedY;
  int Pitch;
  ushort* map;
};

struct CSequence
{
  int numFrames;
  int allocatedFrames;
  CSpriteFrame** f;
};

struct resource::vftable_t
{
  void (__thiscall *)(resource* scalar_deleting_destructor, bool this);
  void (__thiscall *)(resource* scalar_deleting_destructor);
  int (__thiscall *)(resource* scalar_deleting_destructor);
};

struct widget
{
  widget::vftable_union_t vftable;
  heroWindow* parentWindow;
  widget* prevWidget;
  widget* nextWidget;
  int16 id;
  int16 priority;
  int16 style;
  widget::EStatusFlags status;
  int16 x;
  int16 y;
  int16 width;
  int16 height;
  int8* RollOver;
  int8* RightClick;
  bool freeText;
  int sleepCount;
};

struct widget::vftable_t
{
  void* (__thiscall *)(widget* scalar_deleting_destructor, uint this);
  int (__thiscall *)(widget* scalar_deleting_destructor, int this, heroWindow* flags);
  int (__thiscall *)(widget* scalar_deleting_destructor, message* this);
  void (__thiscall *)(widget* scalar_deleting_destructor, ushort* this, int flags);
  void (__thiscall *)(widget* scalar_deleting_destructor);
  int (__thiscall *)(widget* scalar_deleting_destructor);
  int (__thiscall *)(widget* scalar_deleting_destructor);
  void (__thiscall *)(widget* scalar_deleting_destructor);
  void (__thiscall *)(widget* scalar_deleting_destructor);
  void (__thiscall *)(widget* scalar_deleting_destructor, int this);
  void (__thiscall *)(widget* scalar_deleting_destructor);
  void (__thiscall *)(widget* scalar_deleting_destructor);
  void (__thiscall *)(widget* scalar_deleting_destructor, bool this);
};

struct TXT_File
{
  int8* message_1;
  int8* message_2;
  int8* message_3;
  int8* message_4;
  int8* message_5;
  int8* message_6;
  int8* message_7;
  int8* message_8;
  int8* message_9;
  int8* message_10;
  int8* message_11;
  int8* message_12;
  int8* message_13;
  int8* message_14;
  int8* message_15;
  int8* message_16;
  int8* message_17;
  int8* message_18;
  int8* message_19;
  int8* message_20;
  int8* message_21;
  int8* message_22;
  int8* message_23;
  int8* message_24;
  int8* message_25;
  int8* message_26;
  int8* message_27;
  int8* message_28;
  int8* message_29;
  int8* message_30;
  int8* message_31;
  int8* message_32;
  int8* message_33;
  int8* message_34;
  int8* message_35;
  int8* message_36;
  int8* message_37;
  int8* message_38;
  int8* message_39;
  int8* message_40;
  int8* message_41;
  int8* message_42;
  int8* message_43;
  int8* message_44;
  int8* message_45;
  int8* message_46;
  int8* message_47;
  int8* message_48;
  int8* message_49;
  int8* message_50;
  int8* message_51;
  int8* message_52;
  int8* message_53;
  int8* message_54;
  int8* message_55;
  int8* message_56;
  int8* message_57;
  int8* message_58;
  int8* message_59;
  int8* message_60;
  int8* message_61;
  int8* message_62;
  int8* message_63;
  int8* message_64;
  int8* message_65;
  int8* message_66;
  int8* message_67;
  int8* message_68;
  int8* message_69;
  int8* message_70;
  int8* message_71;
  int8* message_72;
  int8* message_73;
  int8* message_74;
  int8* message_75;
  int8* message_76;
  int8* message_77;
  int8* message_78;
  int8* message_79;
  int8* message_80;
  int8* message_81;
  int8* message_82;
  int8* message_83;
  int8* message_84;
  int8* message_85;
  int8* message_86;
  int8* message_87;
  int8* message_88;
  int8* message_89;
  int8* message_90;
  int8* message_91;
  int8* message_92;
  int8* message_93;
  int8* message_94;
  int8* message_95;
  int8* message_96;
  int8* message_97;
  int8* message_98;
  int8* message_99;
  int8* message_100;
  int8* message_101;
  int8* message_102;
  int8* message_103;
  int8* message_104;
  int8* message_105;
  int8* message_106;
  int8* message_107;
  int8* message_108;
  int8* message_109;
  int8* message_110;
  int8* message_111;
  int8* message_112;
  int8* message_113;
  int8* message_114;
  int8* message_115;
  int8* message_116;
  int8* message_117;
  int8* message_118;
  int8* message_119;
  int8* message_120;
  int8* message_121;
  int8* message_122;
  int8* message_123;
  int8* message_124;
  int8* message_125;
  int8* message_126;
  int8* message_127;
  int8* message_128;
  int8* message_129;
  int8* message_130;
  int8* message_131;
  int8* message_132;
  int8* message_133;
  int8* message_134;
  int8* message_135;
  int8* message_136;
  int8* message_137;
  int8* message_138;
  int8* message_139;
  int8* message_140;
  int8* message_141;
  int8* message_142;
  int8* message_143;
  int8* message_144;
  int8* message_145;
  int8* message_146;
  int8* message_147;
  int8* message_148;
  int8* message_149;
  int8* message_150;
  int8* message_151;
  int8* message_152;
  int8* message_153;
  int8* message_154;
  int8* message_155;
  int8* message_156;
  int8* message_157;
  int8* message_158;
  int8* message_159;
  int8* message_160;
  int8* message_161;
  int8* message_162;
  int8* message_163;
  int8* message_164;
  int8* message_165;
  int8* message_166;
  int8* message_167;
  int8* message_168;
  int8* message_169;
  int8* message_170;
  int8* message_171;
  int8* message_172;
  int8* message_173;
  int8* message_174;
  int8* message_175;
  int8* message_176;
  int8* message_177;
  int8* message_178;
  int8* message_179;
  int8* message_180;
  int8* message_181;
  int8* message_182;
  int8* message_183;
  int8* message_184;
  int8* message_185;
  int8* message_186;
  int8* message_187;
  int8* message_188;
  int8* message_189;
  int8* message_190;
  int8* message_191;
  int8* message_192;
  int8* message_193;
  int8* message_194;
  int8* message_195;
  int8* message_196;
  int8* message_197;
  int8* message_198;
  int8* message_199;
  int8* message_200;
  int8* message_201;
  int8* message_202;
  int8* message_203;
  int8* message_204;
  int8* message_205;
  int8* message_206;
  int8* message_207;
  int8* message_208;
  int8* message_209;
  int8* message_210;
  int8* message_211;
  int8* message_212;
  int8* message_213;
  int8* message_214;
  int8* message_215;
  int8* message_216;
  int8* message_217;
  int8* message_218;
  int8* message_219;
  int8* message_220;
  int8* message_221;
  int8* message_222;
  int8* message_223;
  int8* message_224;
  int8* message_225;
  int8* message_226;
  int8* message_227;
  int8* message_228;
  int8* message_229;
  int8* message_230;
  int8* message_231;
  int8* message_232;
  int8* message_233;
  int8* message_234;
  int8* message_235;
  int8* message_236;
  int8* message_237;
  int8* message_238;
  int8* message_239;
  int8* message_240;
  int8* message_241;
  int8* message_242;
  int8* message_243;
  int8* message_244;
  int8* message_245;
  int8* message_246;
  int8* message_247;
  int8* message_248;
  int8* message_249;
  int8* message_250;
  int8* message_251;
  int8* message_252;
  int8* message_253;
  int8* message_254;
  int8* message_255;
  int8* message_256;
  int8* message_257;
  int8* message_258;
  int8* message_259;
  int8* message_260;
  int8* message_261;
  int8* message_262;
  int8* message_263;
  int8* message_264;
  int8* message_265;
  int8* message_266;
  int8* message_267;
  int8* message_268;
  int8* message_269;
  int8* message_270;
  int8* message_271;
  int8* message_272;
  int8* message_273;
  int8* message_274;
  int8* message_275;
  int8* message_276;
  int8* message_277;
  int8* message_278;
  int8* message_279;
  int8* message_280;
  int8* message_281;
  int8* message_282;
  int8* message_283;
  int8* message_284;
  int8* message_285;
  int8* message_286;
  int8* message_287;
  int8* message_288;
  int8* message_289;
  int8* message_290;
  int8* message_291;
  int8* message_292;
  int8* message_293;
  int8* message_294;
  int8* message_295;
  int8* message_296;
  int8* message_297;
  int8* message_298;
  int8* message_299;
  int8* message_300;
  int8* message_301;
  int8* message_302;
  int8* message_303;
  int8* message_304;
  int8* message_305;
  int8* message_306;
  int8* message_307;
  int8* message_308;
  int8* message_309;
  int8* message_310;
  int8* message_311;
  int8* message_312;
  int8* message_313;
  int8* message_314;
  int8* message_315;
  int8* message_316;
  int8* message_317;
  int8* message_318;
  int8* message_319;
  int8* message_320;
  int8* message_321;
  int8* message_322;
  int8* message_323;
  int8* message_324;
  int8* message_325;
  int8* message_326;
  int8* message_327;
  int8* message_328;
  int8* message_329;
  int8* message_330;
  int8* message_331;
  int8* message_332;
  int8* message_333;
  int8* message_334;
  int8* message_335;
  int8* message_336;
  int8* message_337;
  int8* message_338;
  int8* message_339;
  int8* message_340;
  int8* message_341;
  int8* message_342;
  int8* message_343;
  int8* message_344;
  int8* message_345;
  int8* message_346;
  int8* message_347;
  int8* message_348;
  int8* message_349;
  int8* message_350;
  int8* message_351;
  int8* message_352;
  int8* message_353;
  int8* message_354;
  int8* message_355;
  int8* message_356;
  int8* message_357;
  int8* message_358;
  int8* message_359;
  int8* message_360;
  int8* message_361;
  int8* message_362;
  int8* message_363;
  int8* message_364;
  int8* message_365;
  int8* message_366;
  int8* message_367;
  int8* message_368;
  int8* message_369;
  int8* message_370;
  int8* message_371;
  int8* message_372;
  int8* message_373;
  int8* message_374;
  int8* message_375;
  int8* message_376;
  int8* message_377;
  int8* message_378;
  int8* message_379;
  int8* message_380;
  int8* message_381;
  int8* message_382;
  int8* message_383;
  int8* message_384;
  int8* message_385;
  int8* message_386;
  int8* message_387;
  int8* message_388;
  int8* message_389;
  int8* message_390;
  int8* message_391;
  int8* message_392;
  int8* message_393;
  int8* message_394;
  int8* message_395;
  int8* message_396;
  int8* message_397;
  int8* message_398;
  int8* message_399;
  int8* message_400;
  int8* message_401;
  int8* message_402;
  int8* message_403;
  int8* message_404;
  int8* message_405;
  int8* message_406;
  int8* message_407;
  int8* message_408;
  int8* message_409;
  int8* message_410;
  int8* message_411;
  int8* message_412;
  int8* message_413;
  int8* message_414;
  int8* message_415;
  int8* message_416;
  int8* message_417;
  int8* message_418;
  int8* message_419;
  int8* message_420;
  int8* message_421;
  int8* message_422;
  int8* message_423;
  int8* message_424;
  int8* message_425;
  int8* message_426;
  int8* message_427;
  int8* message_428;
  int8* message_429;
  int8* message_430;
  int8* message_431;
  int8* message_432;
  int8* message_433;
  int8* message_434;
  int8* message_435;
  int8* message_436;
  int8* message_437;
  int8* message_438;
  int8* message_439;
  int8* message_440;
  int8* message_441;
  int8* message_442;
  int8* message_443;
  int8* message_444;
  int8* message_445;
  int8* message_446;
  int8* message_447;
  int8* message_448;
  int8* message_449;
  int8* message_450;
  int8* message_451;
  int8* message_452;
  int8* message_453;
  int8* message_454;
  int8* message_455;
  int8* message_456;
  int8* message_457;
  int8* message_458;
  int8* message_459;
  int8* message_460;
  int8* message_461;
  int8* message_462;
  int8* message_463;
  int8* message_464;
  int8* message_465;
  int8* message_466;
  int8* message_467;
  int8* message_468;
  int8* message_469;
  int8* message_470;
  int8* message_471;
  int8* message_472;
  int8* message_473;
  int8* message_474;
  int8* message_475;
  int8* message_476;
  int8* message_477;
  int8* message_478;
  int8* message_479;
  int8* message_480;
  int8* message_481;
  int8* message_482;
  int8* message_483;
  int8* message_484;
  int8* message_485;
  int8* message_486;
  int8* message_487;
  int8* message_488;
  int8* message_489;
  int8* message_490;
  int8* message_491;
  int8* message_492;
  int8* message_493;
  int8* message_494;
  int8* message_495;
  int8* message_496;
  int8* message_497;
  int8* message_498;
  int8* message_499;
  int8* message_500;
  int8* message_501;
  int8* message_502;
  int8* message_503;
  int8* message_504;
  int8* message_505;
  int8* message_506;
  int8* message_507;
  int8* message_508;
  int8* message_509;
  int8* message_510;
  int8* message_511;
  int8* message_512;
  int8* message_513;
  int8* message_514;
  int8* message_515;
  int8* message_516;
  int8* message_517;
  int8* message_518;
  int8* message_519;
  int8* message_520;
  int8* message_521;
  int8* message_522;
  int8* message_523;
  int8* message_524;
  int8* message_525;
  int8* message_526;
  int8* message_527;
  int8* message_528;
  int8* message_529;
  int8* message_530;
  int8* message_531;
  int8* message_532;
  int8* message_533;
  int8* message_534;
  int8* message_535;
  int8* message_536;
  int8* message_537;
  int8* message_538;
  int8* message_539;
  int8* message_540;
  int8* message_541;
  int8* message_542;
  int8* message_543;
  int8* message_544;
  int8* message_545;
  int8* message_546;
  int8* message_547;
  int8* message_548;
  int8* message_549;
  int8* message_550;
  int8* message_551;
  int8* message_552;
  int8* message_553;
  int8* message_554;
  int8* message_555;
  int8* message_556;
  int8* message_557;
  int8* message_558;
  int8* message_559;
  int8* message_560;
  int8* message_561;
  int8* message_562;
  int8* message_563;
  int8* message_564;
  int8* message_565;
  int8* message_566;
  int8* message_567;
  int8* message_568;
  int8* message_569;
  int8* message_570;
  int8* message_571;
  int8* message_572;
  int8* message_573;
  int8* message_574;
  int8* message_575;
  int8* message_576;
  int8* message_577;
  int8* message_578;
  int8* message_579;
  int8* message_580;
  int8* message_581;
  int8* message_582;
  int8* message_583;
  int8* message_584;
  int8* message_585;
  int8* message_586;
  int8* message_587;
  int8* message_588;
  int8* message_589;
  int8* message_590;
  int8* message_591;
  int8* message_592;
  int8* message_593;
  int8* message_594;
  int8* message_595;
  int8* message_596;
  int8* message_597;
  int8* message_598;
  int8* message_599;
  int8* message_600;
  int8* message_601;
  int8* message_602;
  int8* message_603;
  int8* message_604;
  int8* message_605;
  int8* message_606;
  int8* message_607;
  int8* message_608;
  int8* message_609;
  int8* message_610;
  int8* message_611;
  int8* message_612;
  int8* message_613;
  int8* message_614;
  int8* message_615;
  int8* message_616;
  int8* message_617;
  int8* message_618;
  int8* message_619;
  int8* message_620;
  int8* message_621;
  int8* message_622;
  int8* message_623;
  int8* message_624;
  int8* message_625;
  int8* message_626;
  int8* message_627;
  int8* message_628;
  int8* message_629;
  int8* message_630;
  int8* message_631;
  int8* message_632;
  int8* message_633;
  int8* message_634;
  int8* message_635;
  int8* message_636;
  int8* message_637;
  int8* message_638;
  int8* message_639;
  int8* message_640;
  int8* message_641;
  int8* message_642;
  int8* message_643;
  int8* message_644;
  int8* message_645;
  int8* message_646;
  int8* message_647;
  int8* message_648;
  int8* message_649;
  int8* message_650;
  int8* message_651;
  int8* message_652;
  int8* message_653;
  int8* message_654;
  int8* message_655;
  int8* message_656;
  int8* message_657;
  int8* message_658;
  int8* message_659;
  int8* message_660;
  int8* message_661;
  int8* message_662;
  int8* message_663;
  int8* message_664;
  int8* message_665;
  int8* message_666;
  int8* message_667;
  int8* message_668;
  int8* message_669;
  int8* message_670;
  int8* message_671;
  int8* message_672;
  int8* message_673;
  int8* message_674;
  int8* message_675;
  int8* message_676;
  int8* message_677;
  int8* message_678;
  int8* message_679;
  int8* message_680;
  int8* message_681;
  int8* message_682;
  int8* message_683;
  int8* message_684;
  int8* message_685;
  int8* message_686;
  int8* message_687;
  int8* message_688;
  int8* message_689;
  int8* message_690;
  int8* message_691;
  int8* message_692;
  int8* message_693;
  int8* message_694;
  int8* message_695;
  int8* message_696;
  int8* message_697;
  int8* message_698;
  int8* message_699;
  int8* message_700;
  int8* message_701;
  int8* message_702;
  int8* message_703;
  int8* message_704;
  int8* message_705;
  int8* message_706;
  int8* message_707;
  int8* message_708;
  int8* message_709;
  int8* message_710;
  int8* message_711;
  int8* message_712;
  int8* message_713;
  int8* message_714;
  int8* message_715;
  int8* message_716;
  int8* message_717;
  int8* message_718;
  int8* message_719;
  int8* message_720;
  int8* message_721;
  int8* message_722;
  int8* message_723;
  int8* message_724;
  int8* message_725;
  int8* message_726;
  int8* message_727;
  int8* message_728;
  int8* message_729;
  int8* message_730;
  int8* message_731;
  int8* message_732;
  int8* message_733;
  int8* message_734;
  int8* message_735;
  int8* message_736;
  int8* message_737;
  int8* message_738;
  int8* message_739;
  int8* message_740;
  int8* message_741;
  int8* message_742;
  int8* message_743;
  int8* message_744;
  int8* message_745;
  int8* message_746;
  int8* message_747;
  int8* message_748;
  int8* message_749;
  int8* message_750;
  int8* message_751;
  int8* message_752;
  int8* message_753;
  int8* message_754;
  int8* message_755;
  int8* message_756;
  int8* message_757;
  int8* message_758;
  int8* message_759;
  int8* message_760;
  int8* message_761;
  int8* message_762;
  int8* message_763;
  int8* message_764;
  int8* message_765;
};

struct armyGroup
{
  int[7] type;
  int[7] amount;
};

enum TSkillMastery
{
  eMasteryInvalid = 0xFFFFFFFF,
  eMasteryNone = 0x100000000,
  eMasteryBasic = 0x100000001,
  eMasteryAdvanced = 0x100000002,
  eMasteryExpert = 0x100000003,
  kNumMasteries = 0x100000004,
};

struct SMonFrameInfo
{
  int16[6] iMissileOffset;
  float[12] fArrowAngle;
  int iExtraNumTroopsXOffset;
  int iAttackFrames;
  int iFidgetFrequency;
  int iWalkCycleTime;
  int iAttackStartCycleTime;
  int iFlightPixelSpan;
};

enum TTownType
{
  eTownNeutral = 0xFFFFFFFF,
  eTownCastle = 0x100000000,
  eTownRampart = 0x100000001,
  eTownTower = 0x100000002,
  eTownInferno = 0x100000003,
  eTownNecropolis = 0x100000004,
  eTownDungeon = 0x100000005,
  eTownStronghold = 0x100000006,
  eTownFortress = 0x100000007,
  eTownConflux = 0x100000008,
  kNumTownTypes = 0x100000009,
};

struct TCreatureTypeTraits
{
  TTownType townType;
  int level;
  int8* cSamplePrefix;
  int8* m_sprite_name;
  creature_flags attributes;
  int8* m_name;
  int8* m_plural_name;
  int8* special_ability;
  int[7] cost;
  int baseFightValue;
  int AI_value;
  int growthRate;
  int horde_growth_rate;
  int hitPoints;
  int speed;
  int attackSkill;
  int defenseSkill;
  int damageLowBound;
  int damageHighBound;
  int numShots;
  int hasSpell;
  int wanderingLow;
  int wanderingHigh;
};

struct std::vector
{
  int8 allocator;
  void* first;
  void* last;
  void* end;
};

struct std::deque_SpellID_::iterator
{
  SpellID* first;
  SpellID* last;
  SpellID* next;
  SpellID** map;
};

struct std::deque_SpellID_
{
  int8 allocator;
  std::deque_SpellID_::iterator first;
  std::deque_SpellID_::iterator last;
  SpellID* map;
  uint mapsize;
  uint size;
};

enum SpellID
{
  SPELL_NONE = 0xFFFFFFFF,
  K_SPELL_COUNT = 0x100000051,
  K_SPELL_MAX = 0x100000051,
  K_HERO_SPELLS_MAX = 0x200000046,
  SPELL_SUMMON_BOAT = 0x300000000,
  SPELL_SCUTTLE_BOAT = 0x300000001,
  SPELL_VISIONS = 0x300000002,
  SPELL_VIEW_EARTH = 0x300000003,
  SPELL_DISGUISE = 0x300000004,
  SPELL_VIEW_AIR = 0x300000005,
  SPELL_FLY = 0x300000006,
  SPELL_WATER_WALK = 0x300000007,
  SPELL_DIMENSION_DOOR = 0x300000008,
  SPELL_TOWN_PORTAL = 0x300000009,
  SPELL_QUICKSAND = 0x30000000A,
  SPELL_LAND_MINE = 0x30000000B,
  SPELL_FORCE_FIELD = 0x30000000C,
  SPELL_FIRE_WALL = 0x30000000D,
  SPELL_EARTHQUAKE = 0x30000000E,
  SPELL_MAGIC_ARROW = 0x30000000F,
  SPELL_ICE_BOLT = 0x300000010,
  SPELL_LIGHTNING_BOLT = 0x300000011,
  SPELL_IMPLOSION = 0x300000012,
  SPELL_CHAIN_LIGHTNING = 0x300000013,
  SPELL_FROST_RING = 0x300000014,
  SPELL_FIREBALL = 0x300000015,
  SPELL_INFERNO = 0x300000016,
  SPELL_METEOR_SHOWER = 0x300000017,
  SPELL_DEATH_RIPPLE = 0x300000018,
  SPELL_DESTROY_UNDEAD = 0x300000019,
  SPELL_ARMAGEDDON = 0x30000001A,
  SPELL_SHIELD = 0x30000001B,
  SPELL_AIR_SHIELD = 0x30000001C,
  SPELL_FIRE_SHIELD = 0x30000001D,
  SPELL_PROTECTION_FROM_AIR = 0x30000001E,
  SPELL_PROTECTION_FROM_FIRE = 0x30000001F,
  SPELL_PROTECTION_FROM_WATER = 0x300000020,
  SPELL_PROTECTION_FROM_EARTH = 0x300000021,
  SPELL_ANTI_MAGIC = 0x300000022,
  SPELL_DISPEL = 0x300000023,
  SPELL_MAGIC_MIRROR = 0x300000024,
  SPELL_CURE = 0x300000025,
  SPELL_RESURRECTION = 0x300000026,
  SPELL_ANIMATE_DEAD = 0x300000027,
  SPELL_SACRIFICE = 0x300000028,
  SPELL_BLESS = 0x300000029,
  SPELL_CURSE = 0x30000002A,
  SPELL_BLOODLUST = 0x30000002B,
  SPELL_PRECISION = 0x30000002C,
  SPELL_WEAKNESS = 0x30000002D,
  SPELL_STONE_SKIN = 0x30000002E,
  SPELL_DISRUPTING_RAY = 0x30000002F,
  SPELL_PRAYER = 0x300000030,
  SPELL_MIRTH = 0x300000031,
  SPELL_SORROW = 0x300000032,
  SPELL_FORTUNE = 0x300000033,
  SPELL_MISFORTUNE = 0x300000034,
  SPELL_HASTE = 0x300000035,
  SPELL_SLOW = 0x300000036,
  SPELL_SLAYER = 0x300000037,
  SPELL_FRENZY = 0x300000038,
  SPELL_TITANS_LIGHTNING_BOLT = 0x300000039,
  SPELL_COUNTERSTRIKE = 0x30000003A,
  SPELL_BERSERK = 0x30000003B,
  SPELL_HYPNOTIZE = 0x30000003C,
  SPELL_FORGETFULNESS = 0x30000003D,
  SPELL_BLIND = 0x30000003E,
  SPELL_TELEPORT = 0x30000003F,
  SPELL_REMOVE_OBSTACLE = 0x300000040,
  SPELL_CLONE = 0x300000041,
  SPELL_FIRE_ELEMENTAL = 0x300000042,
  SPELL_EARTH_ELEMENTAL = 0x300000043,
  SPELL_WATER_ELEMENTAL = 0x300000044,
  SPELL_AIR_ELEMENTAL = 0x300000045,
  SPELL_STONE = 0x300000046,
  SPELL_POISON = 0x300000047,
  SPELL_BIND = 0x300000048,
  SPELL_DESEASE = 0x300000049,
  SPELL_PARALYZE = 0x30000004A,
  SPELL_AGE = 0x30000004B,
  SPELL_DEATH_CLOUD = 0x30000004C,
  SPELL_THUNDERBOLT = 0x30000004D,
  SPELL_DISPEL_HELPFUL_SPELLS = 0x30000004E,
  SPELL_DEATH_STARE = 0x30000004F,
  SPELL_ACID_BREATH = 0x300000050,
};

enum TCreatureType
{
  CREATURE_PIKEMAN = 0x0,
  CREATURE_HALBERDIER = 0x1,
  CREATURE_ARCHER = 0x2,
  CREATURE_MARKSMAN = 0x3,
  CREATURE_GRIFFIN = 0x4,
  CREATURE_ROYAL_GRIFFIN = 0x5,
  CREATURE_SWORDSMAN = 0x6,
  CREATURE_CRUSADER = 0x7,
  CREATURE_MONK = 0x8,
  CREATURE_ZEALOT = 0x9,
  CREATURE_CAVALIER = 0xA,
  CREATURE_CHAMPION = 0xB,
  CREATURE_ANGEL = 0xC,
  CREATURE_ARCHANGEL = 0xD,
  CREATURE_CENTAUR = 0xE,
  CREATURE_CENTAUR_CAPTAIN = 0xF,
  CREATURE_DWARF = 0x10,
  CREATURE_BATTLE_DWARF = 0x11,
  CREATURE_WOOD_ELF = 0x12,
  CREATURE_GRAND_ELF = 0x13,
  CREATURE_PEGASUS = 0x14,
  CREATURE_SILVER_PEGASUS = 0x15,
  CREATURE_DENDROID_GUARD = 0x16,
  CREATURE_DENDROID_SOLDIER = 0x17,
  CREATURE_UNICORN = 0x18,
  CREATURE_WAR_UNICORN = 0x19,
  CREATURE_GREEN_DRAGON = 0x1A,
  CREATURE_GOLD_DRAGON = 0x1B,
  CREATURE_GREMLIN = 0x1C,
  CREATURE_MASTER_GREMLIN = 0x1D,
  CREATURE_STONE_GARGOYLE = 0x1E,
  CREATURE_OBSIDIAN_GARGOYLE = 0x1F,
  CREATURE_STONE_GOLEM = 0x20,
  CREATURE_IRON_GOLEM = 0x21,
  CREATURE_MAGE = 0x22,
  CREATURE_ARCH_MAGE = 0x23,
  CREATURE_GENIE = 0x24,
  CREATURE_MASTER_GENIE = 0x25,
  CREATURE_NAGA = 0x26,
  CREATURE_NAGA_QUEEN = 0x27,
  CREATURE_GIANT = 0x28,
  CREATURE_TITAN = 0x29,
  CREATURE_IMP = 0x2A,
  CREATURE_FAMILIAR = 0x2B,
  CREATURE_GOG = 0x2C,
  CREATURE_MAGOG = 0x2D,
  CREATURE_HELL_HOUND = 0x2E,
  CREATURE_CERBERUS = 0x2F,
  CREATURE_DEMON = 0x30,
  CREATURE_HORNED_DEMON = 0x31,
  CREATURE_PIT_FIEND = 0x32,
  CREATURE_PIT_LORD = 0x33,
  CREATURE_EFREETI = 0x34,
  CREATURE_EFREET_SULTAN = 0x35,
  CREATURE_DEVIL = 0x36,
  CREATURE_ARCH_DEVIL = 0x37,
  CREATURE_SKELETON = 0x38,
  CREATURE_SKELETON_WARRIOR = 0x39,
  CREATURE_WALKING_DEAD = 0x3A,
  CREATURE_ZOMBIE = 0x3B,
  CREATURE_WIGHT = 0x3C,
  CREATURE_WRAITH = 0x3D,
  CREATURE_VAMPIRE = 0x3E,
  CREATURE_VAMPIRE_LORD = 0x3F,
  CREATURE_LICH = 0x40,
  CREATURE_POWER_LICH = 0x41,
  CREATURE_BLACK_KNIGHT = 0x42,
  CREATURE_DREAD_KNIGHT = 0x43,
  CREATURE_BONE_DRAGON = 0x44,
  CREATURE_GHOST_DRAGON = 0x45,
  CREATURE_TROGLODYTE = 0x46,
  CREATURE_INFERNAL_TROGLODYTE = 0x47,
  CREATURE_HARPY = 0x48,
  CREATURE_HARPY_HAG = 0x49,
  CREATURE_BEHOLDER = 0x4A,
  CREATURE_EVIL_EYE = 0x4B,
  CREATURE_MEDUSA = 0x4C,
  CREATURE_MEDUSA_QUEEN = 0x4D,
  CREATURE_MINOTAUR = 0x4E,
  CREATURE_MINOTAUR_KING = 0x4F,
  CREATURE_MANTICORE = 0x50,
  CREATURE_SCORPICORE = 0x51,
  CREATURE_RED_DRAGON = 0x52,
  CREATURE_BLACK_DRAGON = 0x53,
  CREATURE_GOBLIN = 0x54,
  CREATURE_HOBGOBLIN = 0x55,
  CREATURE_WOLF_RIDER = 0x56,
  CREATURE_WOLF_RAIDER = 0x57,
  CREATURE_ORC = 0x58,
  CREATURE_ORC_CHIEFTAIN = 0x59,
  CREATURE_OGRE = 0x5A,
  CREATURE_OGRE_MAGE = 0x5B,
  CREATURE_ROC = 0x5C,
  CREATURE_THUNDERBIRD = 0x5D,
  CREATURE_CYCLOPS = 0x5E,
  CREATURE_CYCLOPS_KING = 0x5F,
  CREATURE_BEHEMOTH = 0x60,
  CREATURE_ANCIENT_BEHEMOTH = 0x61,
  CREATURE_GNOLL = 0x62,
  CREATURE_GNOLL_MARAUDER = 0x63,
  CREATURE_LIZARDMAN = 0x64,
  CREATURE_LIZARD_WARRIOR = 0x65,
  CREATURE_GORGON = 0x66,
  CREATURE_MIGHTY_GORGON = 0x67,
  CREATURE_SERPENT_FLY = 0x68,
  CREATURE_DRAGON_FLY = 0x69,
  CREATURE_BASILISK = 0x6A,
  CREATURE_GREATER_BASILISK = 0x6B,
  CREATURE_WYVERN = 0x6C,
  CREATURE_WYVERN_MONARCH = 0x6D,
  CREATURE_HYDRA = 0x6E,
  CREATURE_CHAOS_HYDRA = 0x6F,
  CREATURE_AIR_ELEMENTAL = 0x70,
  CREATURE_EARTH_ELEMENTAL = 0x71,
  CREATURE_FIRE_ELEMENTAL = 0x72,
  CREATURE_WATER_ELEMENTAL = 0x73,
  CREATURE_GOLD_GOLEM = 0x74,
  CREATURE_DIAMOND_GOLEM = 0x75,
  CREATURE_PIXIE = 0x76,
  CREATURE_SPRITE = 0x77,
  CREATURE_PSYCHIC_ELEMENTAL = 0x78,
  CREATURE_MAGIC_ELEMENTAL = 0x79,
  CREATURE_122 = 0x7A,
  CREATURE_ICE_ELEMENTAL = 0x7B,
  CREATURE_124 = 0x7C,
  CREATURE_MAGMA_ELEMENTAL = 0x7D,
  CREATURE_126 = 0x7E,
  CREATURE_STORM_ELEMENTAL = 0x7F,
  CREATURE_128 = 0x80,
  CREATURE_ENERGY_ELEMENTAL = 0x81,
  CREATURE_FIREBIRD = 0x82,
  CREATURE_PHOENIX = 0x83,
  CREATURE_AZURE_DRAGON = 0x84,
  CREATURE_CRYSTAL_DRAGON = 0x85,
  CREATURE_FAERIE_DRAGON = 0x86,
  CREATURE_RUST_DRAGON = 0x87,
  CREATURE_ENCHANTER = 0x88,
  CREATURE_SHARPSHOOTER = 0x89,
  CREATURE_HALFLING = 0x8A,
  CREATURE_PEASANT = 0x8B,
  CREATURE_BOAR = 0x8C,
  CREATURE_MUMMY = 0x8D,
  CREATURE_NOMAD = 0x8E,
  CREATURE_ROGUE = 0x8F,
  CREATURE_TROLL = 0x90,
  CREATURE_CATAPULT = 0x91,
  CREATURE_BALLISTA = 0x92,
  CREATURE_FIRST_AID_TENT = 0x93,
  CREATURE_AMMO_CART = 0x94,
  CREATURE_ARROW_TOWER = 0x95,
  CREATURE_NONE = 0xFFFFFFFF,
  CREATURE_ROE_CATAPULT = 0x100000076,
  CREATURE_ROE_BALLISTA = 0x100000077,
  CREATURE_ROE_FIRST_AID_TENT = 0x100000078,
  CREATURE_ROE_AMMO_CART = 0x100000079,
  MAX_CREATURES_ROE = 0x200000076,
  MAX_CREATURES_AB = 0x200000091,
  MAX_CREATURES_SOD = 0x200000091,
  MAX_CREATURES = 0x200000091,
  MAX_COMBAT_CREATURES_ROE = 0x30000007A,
  MAX_COMBAT_CREATURES_AB = 0x300000096,
  MAX_COMBAT_CREATURES_SOD = 0x300000096,
  MAX_COMBAT_CREATURES = 0x300000096,
};

struct army
{
  bool bShowAttackFrames;
  bool bShowRangeFrames;
  int8 iShowAttackFrameType;
  int8 iNextFrameType;
  int8 iRemainingFrames;
  int iDrawPriority;
  bool bShowTroopCount;
  int groupToAttack;
  int indexToAttack;
  int attackLimit;
  int targetCellIndex;
  bool bShowPowEffect;
  int iMirrorSourceIndex;
  int iMirrorDestIndex;
  int iRoundsLeftBeforeVanish;
  bool IsMoving;
  int8 LetsPretendImNotHere;
  TCreatureType armyType;
  int gridIndex;
  int currFrameType;
  int currFrameIndex;
  int facing;
  int walkDirection;
  int numTroops;
  int numTroopsToShowOverride;
  int numTroopsBattleResurrected;
  int residualDamage;
  int origPos;
  int origNumTroops;
  int origSpeed;
  int origWalkCycleTime;
  int origHitPoints;
  int iLuckStatus;
  TCreatureTypeTraits sMonInfo;
  bool show_fire_shield;
  bool bSomeUnitsDamaged;
  bool bAllUnitsKilled;
  SpellID iPostPowSpellToCast;
  bool hitByCreature;
  int group;
  int index;
  unsigned int iLastFidgetTime;
  int ySpecialMod;
  int xSpecialMod;
  int bPowSequenceComplete;
  int yModify;
  SMonFrameInfo sMonFrameInfo;
  CSprite* stdIcon;
  CSprite* missileIcon;
  int image_height;
  sample*[8] armySample;
  unsigned int expected_move_order;
  int numSpellInfluences;
  int[81] spellInfluence;
  TSkillMastery[81] spell_level;
  std::deque_SpellID_ SpellInfluenceQueue;
  float PaletteEffect;
  int retaliationCount;
  unsigned int blessFactor;
  unsigned int curseFactor;
  int antiMagicSpellLevel;
  int bloodlustBonus;
  int precisionBonus;
  int weaknessPenalty;
  int toughskinBonus;
  int disruptiverayPenalty;
  int prayerBonus;
  int mirthBonus;
  int sorrowPenalty;
  int fortuneBonus;
  int misfortunePenalty;
  int slayerLevel;
  int joustBonus;
  int counterstrokeBonus;
  float frenzyAdjust;
  float blindFactor;
  float fire_shield_strength;
  float poison_penalty;
  float protectionFromAirFactor;
  float protectionFromFireFactor;
  float protectionFromWaterFactor;
  float protectionFromEarthFactor;
  float shieldDamageFactor;
  float airShieldDamageFactor;
  bool residualBlindness;
  bool residualParalyze;
  TSkillMastery forgetfulness_level;
  float slowPenalty;
  int tailwindBonus;
  int diseaseDefensePenalty;
  int diseaseAttackPenalty;
  bool OnNativeTerrain;
  int DefendBonus;
  int faerieDragonSpell;
  unsigned int backlash_chance;
  int iMorale;
  int iLuck;
  bool reset_this_round;
  bool is_area_effect_target;
  std::vector_army_ptr_ bound_armies;
  std::vector_army_ptr_ binders;
  std::vector_army_ptr_ aura_clients;
  std::vector_army_ptr_ aura_sources;
  unsigned int AI_expected_damage;
  army* AI_target;
  unsigned int AI_target_value;
  unsigned int AI_target_distance;
  unsigned int AI_possible_targets;
};

struct resource
{
  resource::vftable_union_t vftable;
  int8[13] Name;
  int resType;
  int ReferenceCount;
};

enum TEncodingMethod
{
  eEncodeRaw = 0x0,
  eEncodeGeneralRLE = 0x1,
  eEncodeTilesetRLE = 0x2,
  eEncodeAdvObjRLE = 0x3,
  kNumEncodingMethods = 0x4,
};

unsigned int8;

struct Bitmap816 : resource
{
  int DataSize;
  int ImageSize;
  int Width;
  int Height;
  int Pitch;
  ushort* map;
  TPalette16 Palette;
  TPalette24 Palette24;
};

struct ResourceManager { };

unsigned int16;

struct Bitmap16Bit : resource
{
  int DataSize;
  int ImageSize;
  int Width;
  int Height;
  int Pitch;
  uchar* map;
  bool keepData;
};

unsigned int;

struct font::TFontSpec::myABC
{
  int abcA;
  int abcB;
  int abcC;
};

unsigned int;

struct font::TFontSpec
{
  uchar first;
  uchar last;
  uchar depth;
  int8 xspace;
  int8 yspace;
  uchar height;
  int8 baseyoffset;
  int8 pad;
  int numpal;
  ushort[10] pal;
  font::TFontSpec::myABC[256] abc;
  ulong[256] Offset;
};

struct font : resource
{
  font::TFontSpec fr;
  TPalette16 p16;
  uchar* Data;
  uint DataSize;
};

struct advManager : baseManager
{
  CNetMsgHandler* pNetMsgHandler;
  bool DebugShowFPS;
  bool DebugViewAll;
  int advCommand;
  TAdventureMapWindow* advWindow;
  ushort* pRouteArray;
  int bShowRoute;
  int seedingValid;
  int fullySeeded;
  int lastTerrain;
  NewfullMap* map;
  CSprite*[10] groundTileset;
  CSprite*[5] riverTileset;
  CSprite*[4] roadTileset;
  CSprite* borderTileset;
  CSprite* arrowTileset;
  CSprite*[4] gemIcons;
  CSprite* starTileset;
  CSprite* radarIcons;
  CSprite* cloudIcons;
  std::vector_resource_ptr_ CachedGraphics;
  CSprite* monAttackSprites;
  type_point map_origin;
  type_point last_map_hover;
  int lastHoverX;
  int lastHoverY;
  int scrollX;
  int scrollY;
  int animFrame;
  int animCtr;
  bool animCtrPaused;
  int flagFrame;
  CSprite*[18] cursorIcons;
  CSprite*[3] boatIcons;
  CSprite*[3] boatFrothIcons;
  CSprite*[8] flagIcons;
  CSprite*[24] boatFlagIcons;
  bool heroVisible;
  int heroType;
  int heroDirection;
  int heroBaseFrame;
  int heroSequence;
  int heroFrameCount;
  int heroTurning;
  int heroDrawn;
  bool bCurHeroMobile;
  int iShowMode;
  int bForceCompleteDraw;
  int monAttackObjIndex;
  int monAttackSpriteIndex;
  int monAttackFlip;
  int touchedSounds;
  soundNode[4] soundArray;
  sample*[70] loopedSample;
  sample*[11] heroSamples;
  int bHeroLogoShowing;
  uchar bHeroMoving;
  advManager::EBottomViewType CurrentBottomView;
  advManager::EBottomViewType BottomViewOverride;
  int BottomViewOverrideEndTime;
  int BottomViewResource;
  int BottomViewResourceQty;
  std::string BottomViewText;
};

struct game
{
  heroWindow* newGameWin;
  uchar[70] spellAllocInfo;
  uchar[70] spellDisabledInfo;
  LPCRITICAL_SECTION bink_critical_section;
  std::vector_TownExtra_ townExtraPool;
  HeroExtra[156] heroExtraPool;
  int difficultyRating;
  SCampaign sCampaign;
  bool bNewCampaignStarted;
  int8[351] cGameFilename;
  int8 numPlayers;
  int8 numDeadPlayers;
  bool[8] playerDead;
  ushort day;
  ushort week;
  ushort month;
  int8[32] cUniqueSystemID;
  TArtifact[7] marketArtifacts;
  std::vector_TBlackMarket_ BlackMarkets;
  int16 ultimateX;
  int16 ultimateY;
  uchar ultimateZ;
  uchar ultimateRadius;
  uchar ultimateValid;
  byte[1] align8;
  int iGameType;
  bool bIsCheater;
  bool is_tutorial;
  byte[2] align9;
  SGameSetupOptions sSetup;
  NewSMapHeader sMapHeader;
  NewfullMap worldMap;
  int[1] align10;
  playerData[8] player;
  std::vector_town_ townPool;
  hero[156] heroPool;
  int8[156] heroAllocInfo;
  std::bitset_8_[156] heroAvailable;
  int8[144] artifactAllocInfo;
  int8[144] reservedArtifactInfo;
  uchar[32] InfoFlags;
  uchar[8] GuardFlags;
  ushort[3] cartographerMask;
  uchar[3] cartographerFlags;
  byte[3] aligned11;
  std::vector_Sign_ signPool;
  std::vector_mine_ minePool;
  std::vector_generator_ generatorPool;
  std::vector_garrison_ garrisonPool;
  std::vector_boat_ boatPool;
  std::vector_type_university_ university_pool;
  std::vector_type_creature_bank_ creature_banks;
  int8 numObelisks;
  int8[48] obeliskPool;
  int8[300] cCurRumour;
  byte[1] aligned12;
  bool[256] rumourAllocInfo;
  byte[2] aligned13;
  std::vector_game__TRumour_ MapRumours;
  bool[28] ss_disabled;
  heroWindow* armyWindow;
  byte[4] aligned14;
  std::vector_type_point_[8] two_way_liths;
  std::vector_type_point_[8] lith_exits;
  std::vector_type_point_ whirlpools;
  std::vector_type_point_ underground_gates;
  std::vector_type_point_ underground_gate_exits;
  std::vector_type_event_record_ptr_ recorded_events;
  std::vector_QuestMonster_ quest_monsters;
  byte[4] aligned15;
};

struct bitmapBorder : border
{
  Bitmap816* borderBitmap16;
};

struct iconWidget : widget
{
  CSprite* Sprite;
  int Frame;
  int seqId;
  uchar IsFlipped;
  int PostPostWalkSequence;
  ushort BackColor;
};

struct heroWindow
{
  heroWindow::vftable_union_t vftable;
  int priority;
  heroWindow* nextWindow;
  heroWindow* prevWindow;
  heroWindow::TAttribute type;
  heroWindow::TStatus status;
  int x;
  int y;
  int width;
  int height;
  widget* headWidget;
  widget* tailWidget;
  TWidgetVector Widgets;
  int focusId;
  Bitmap16Bit* background;
  int sleepCount;
};

struct TAdventureMapWindow : heroWindow
{
  widget* RadarWidget;
  widget* MapWidget;
  textWidget* ChatTextWidget;
  CChatEdit* chatEdit;
  TResourceDisplay* ResourceDisplay;
  int topHero;
  int topTown;
  bitmapBackedTextWidget* RolloverWidget;
  bool animate_in_background;
  bitmapBorder*[5] hero_borders;
  bitmapBorder*[5] hero_highlight_borders;
  type_bottom_view_window* bottom_view;
  void* immersion_ptr;
};

struct border : widget { };

struct button : widget
{
  CSprite* buttonIcon;
  button::TButtonStates normalFrame;
  button::TButtonStates selectedFrame;
  button::TButtonStates disabled_frame;
  button::TButtonStates highlightedFrame;
  bool _end;
  std::vector_int_ hotKeyCodes;
  std::string Text;
};

struct CGameChatEdit : CChatEdit
{
  bool activated;
};

struct std::string
{
  void* allocator;
  int8* c_str;
  uint length;
  uint capacity;
};

enum font::TColor
{
  PRIMARY = 0x1,
  PRIMARY_HIGHLIGHT = 0x2,
  PRIMARY_DIM = 0x3,
  WHITE = 0x4,
  WHITE_HIGHLIGHT = 0x5,
  WHITE_DIM = 0x6,
  HEADING = 0x7,
  HEADING_HIGHLIGHT = 0x8,
  HEADING_DIM = 0x9,
  WHITE_PLAYER = 0xA,
  WHITE_PLAYER_HIGHLIGHT = 0xB,
  WHITE_PLAYER_DIM = 0xC,
  CHAT = 0xD,
  CHAT_HIGHLIGHT = 0xE,
  CHAT_DIM = 0xF,
  LowestColor = 0x100000001,
  HighestColor = 0x10000000E,
  CUSTOM_COLOR = 0x100000100,
};

enum font::EJustify
{
  LEFT_JUSTIFIED = 0x0,
  CENTER_JUSTIFIED = 0x1,
  RIGHT_JUSTIFIED = 0x2,
  TOP_JUSTIFIED = 0x100000000,
  VERT_CENTER_JUSTIFIED = 0x100000004,
  BOTTOM_JUSTIFIED = 0x100000008,
};

struct textWidget : widget
{
  std::string Text;
  font* Font;
  font::TColor Color;
  font::TColor BackColor;
  font::EJustify Justify;
};

struct textEntryWidget : textWidget
{
  Bitmap816* textBack;
  CTextEntrySave* saveBack;
  ushort cursorIndex;
  ushort bufferSize;
  int16 textWidth;
  int16 textHeight;
  int16 textX;
  int16 textY;
  int16 textLines;
  int16 attributes;
  int16 type;
  int16 displayOffset;
  int8 cursorFlashOn;
  bool focus;
  bool autoDraw;
};

struct type_point
{
  int16 X;
  int16 YZ;
};

enum TAdventureObjectType
{
  OBJECT_ALTAR_OF_SACRIFICE = 0x2,
  OBJECT_ANCHOR_POINT = 0x3,
  OBJECT_ARENA = 0x4,
  OBJECT_ARTIFACT = 0x5,
  OBJECT_PANDORAS_BOX = 0x6,
  OBJECT_BLACK_MARKET = 0x7,
  OBJECT_BOAT = 0x8,
  OBJECT_BORDERGUARD = 0x9,
  OBJECT_KEYMASTER = 0xA,
  OBJECT_BUOY = 0xB,
  OBJECT_CAMPFIRE = 0xC,
  OBJECT_CARTOGRAPHER = 0xD,
  OBJECT_SWAN_POND = 0xE,
  OBJECT_COVER_OF_DARKNESS = 0xF,
  OBJECT_CREATURE_BANK = 0x10,
  OBJECT_CREATURE_GENERATOR1 = 0x11,
  OBJECT_CREATURE_GENERATOR2 = 0x12,
  OBJECT_CREATURE_GENERATOR3 = 0x13,
  OBJECT_CREATURE_GENERATOR4 = 0x14,
  OBJECT_CURSED_GROUND = 0x15,
  OBJECT_CORPSE = 0x16,
  OBJECT_MARLETTO_TOWER = 0x17,
  OBJECT_DERELICT_SHIP = 0x18,
  OBJECT_DRAGON_UTOPIA = 0x19,
  OBJECT_EVENT = 0x1A,
  OBJECT_EYE_OF_MAGI = 0x1B,
  OBJECT_FAERIE_RING = 0x1C,
  OBJECT_FLOTSAM = 0x1D,
  OBJECT_FOUNTAIN_OF_YOUTH = 0x1F,
  OBJECT_FOUNTAIN_O_FORTUNE = 0x10000001E,
  OBJECT_GARDEN_OF_REVELATION = 0x100000020,
  OBJECT_GARRISON = 0x100000021,
  OBJECT_HERO = 0x100000022,
  OBJECT_HILL_FORT = 0x100000023,
  OBJECT_GRAIL = 0x100000024,
  OBJECT_HUT_OF_MAGI = 0x100000025,
  OBJECT_IDOL_OF_FORTUNE = 0x100000026,
  OBJECT_LEAN_TO = 0x100000027,
  OBJECT_DECORATIVE = 0x100000028,
  OBJECT_LIBRARY_OF_ENLIGHTENMENT = 0x100000029,
  OBJECT_LIGHTHOUSE = 0x10000002A,
  OBJECT_MONOLITH_ONE_WAY_ENTRANCE = 0x10000002B,
  OBJECT_MONOLITH_ONE_WAY_EXIT = 0x10000002C,
  OBJECT_MONOLITH_TWO_WAY = 0x10000002D,
  OBJECT_MAGIC_PLAINS = 0x10000002E,
  OBJECT_SCHOOL_OF_MAGIC = 0x10000002F,
  OBJECT_MAGIC_SPRING = 0x100000030,
  OBJECT_MAGIC_WELL = 0x100000031,
  OBJECT_MARKET_OF_TIME = 0x100000032,
  OBJECT_MERCENARY_CAMP = 0x100000033,
  OBJECT_MERMAID = 0x100000034,
  OBJECT_MINE = 0x100000035,
  OBJECT_MONSTER = 0x100000036,
  OBJECT_MYSTICAL_GARDEN = 0x100000037,
  OBJECT_OASIS = 0x100000038,
  OBJECT_OBELISK = 0x100000039,
  OBJECT_REDWOOD_OBSERVATORY = 0x10000003A,
  OBJECT_OCEAN_BOTTLE = 0x10000003B,
  OBJECT_PILLAR_OF_FIRE = 0x10000003C,
  OBJECT_STAR_AXIS = 0x10000003D,
  OBJECT_PRISON = 0x10000003E,
  OBJECT_PYRAMID = 0x10000003F,
  OBJECT_RALLY_FLAG = 0x100000040,
  OBJECT_RANDOM_ART = 0x100000041,
  OBJECT_RANDOM_TREASURE_ART = 0x100000042,
  OBJECT_RANDOM_MINOR_ART = 0x100000043,
  OBJECT_RANDOM_MAJOR_ART = 0x100000044,
  OBJECT_RANDOM_RELIC_ART = 0x100000045,
  OBJECT_RANDOM_HERO = 0x100000046,
  OBJECT_RANDOM_MONSTER = 0x100000047,
  OBJECT_RANDOM_MONSTER_L1 = 0x100000048,
  OBJECT_RANDOM_MONSTER_L2 = 0x100000049,
  OBJECT_RANDOM_MONSTER_L3 = 0x10000004A,
  OBJECT_RANDOM_MONSTER_L4 = 0x10000004B,
  OBJECT_RANDOM_RESOURCE = 0x10000004C,
  OBJECT_RANDOM_TOWN = 0x10000004D,
  OBJECT_REFUGEE_CAMP = 0x10000004E,
  OBJECT_RESOURCE = 0x10000004F,
  OBJECT_SANCTUARY = 0x100000050,
  OBJECT_SCHOLAR = 0x100000051,
  OBJECT_SEA_CHEST = 0x100000052,
  OBJECT_SEER_HUT = 0x100000053,
  OBJECT_CRYPT = 0x100000054,
  OBJECT_SHIPWRECK = 0x100000055,
  OBJECT_SHIPWRECK_SURVIVOR = 0x100000056,
  OBJECT_SHIPYARD = 0x100000057,
  OBJECT_SHRINE_OF_MAGIC_INCANTATION = 0x100000058,
  OBJECT_SHRINE_OF_MAGIC_GESTURE = 0x100000059,
  OBJECT_SHRINE_OF_MAGIC_THOUGHT = 0x10000005A,
  OBJECT_SIGN = 0x10000005B,
  OBJECT_SIRENS = 0x10000005C,
  OBJECT_SPELL_SCROLL = 0x10000005D,
  OBJECT_STABLES = 0x10000005E,
  OBJECT_TAVERN = 0x10000005F,
  OBJECT_TEMPLE = 0x100000060,
  OBJECT_DEN_OF_THIEVES = 0x100000061,
  OBJECT_TOWN = 0x100000062,
  OBJECT_TRADING_POST = 0x100000063,
  OBJECT_LEARNING_STONE = 0x100000064,
  OBJECT_TREASURE_CHEST = 0x100000065,
  OBJECT_TREE_OF_KNOWLEDGE = 0x100000066,
  OBJECT_SUBTERRANEAN_GATE = 0x100000067,
  OBJECT_UNIVERSITY = 0x100000068,
  OBJECT_WAGON = 0x100000069,
  OBJECT_WAR_MACHINE_FACTORY = 0x10000006A,
  OBJECT_SCHOOL_OF_WAR = 0x10000006B,
  OBJECT_WARRIORS_TOMB = 0x10000006C,
  OBJECT_WATER_WHEEL = 0x10000006D,
  OBJECT_WATERING_HOLE = 0x10000006E,
  OBJECT_WHIRLPOOL = 0x10000006F,
  OBJECT_WINDMILL = 0x100000070,
  OBJECT_WITCH_HUT = 0x100000071,
  OBJECT_TERRAIN_BRUSH = 0x100000072,
  OBJECT_TERRAIN_BUSH = 0x100000073,
  OBJECT_TERRAIN_CACTUS = 0x100000074,
  OBJECT_TERRAIN_CANYON = 0x100000075,
  OBJECT_TERRAIN_CRATER = 0x100000076,
  OBJECT_TERRAIN_DEAD_VEGETATION = 0x100000077,
  OBJECT_TERRAIN_FLOWERS = 0x100000078,
  OBJECT_TERRAIN_FROZEN_LAKE = 0x100000079,
  OBJECT_TERRAIN_HEDGE = 0x10000007A,
  OBJECT_TERRAIN_HILL = 0x10000007B,
  OBJECT_TERRAIN_HOLE = 0x10000007C,
  OBJECT_TERRAIN_KELP = 0x10000007D,
  OBJECT_TERRAIN_LAKE = 0x10000007E,
  OBJECT_TERRAIN_LAVA_FLOW = 0x10000007F,
  OBJECT_TERRAIN_LAVA_LAKE = 0x100000080,
  OBJECT_TERRAIN_MUSHROOMS = 0x100000081,
  OBJECT_TERRAIN_LOG = 0x100000082,
  OBJECT_TERRAIN_MANDRAKE = 0x100000083,
  OBJECT_TERRAIN_MOSS = 0x100000084,
  OBJECT_TERRAIN_MOUND = 0x100000085,
  OBJECT_TERRAIN_MOUNTAIN = 0x100000086,
  OBJECT_TERRAIN_OAK_TREES = 0x100000087,
  OBJECT_TERRAIN_OUTCROPPING = 0x100000088,
  OBJECT_TERRAIN_PINE_TREES = 0x100000089,
  OBJECT_TERRAIN_PLANT = 0x10000008A,
  OBJECT_TERRAIN_BLANK1 = 0x10000008B,
  OBJECT_TERRAIN_BLANK2 = 0x10000008C,
  OBJECT_TERRAIN_BLANK3 = 0x10000008D,
  OBJECT_TERRAIN_BLANK4 = 0x10000008E,
  OBJECT_TERRAIN_RIVER_DELTA = 0x10000008F,
  OBJECT_TERRAIN_BLANK5 = 0x100000090,
  OBJECT_TERRAIN_BLANK6 = 0x100000091,
  OBJECT_TERRAIN_BLANK7 = 0x100000092,
  OBJECT_TERRAIN_ROCK = 0x100000093,
  OBJECT_TERRAIN_SAND_DUNE = 0x100000094,
  OBJECT_TERRAIN_SAND_PIT = 0x100000095,
  OBJECT_TERRAIN_SHRUB = 0x100000096,
  OBJECT_TERRAIN_SKULL = 0x100000097,
  OBJECT_TERRAIN_STALAGMITE = 0x100000098,
  OBJECT_TERRAIN_STUMP = 0x100000099,
  OBJECT_TERRAIN_TAR_PIT = 0x10000009A,
  OBJECT_TERRAIN_TREES = 0x10000009B,
  OBJECT_TERRAIN_VINE = 0x10000009C,
  OBJECT_TERRAIN_VOLCANIC_TENT = 0x10000009D,
  OBJECT_TERRAIN_VOLCANO = 0x10000009E,
  OBJECT_TERRAIN_WILLOW_TREES = 0x10000009F,
  OBJECT_TERRAIN_YUCCA_TREES = 0x1000000A0,
  OBJECT_TERRAIN_REEF = 0x1000000A1,
  OBJECT_RANDOM_MONSTER_L5 = 0x1000000A2,
  OBJECT_RANDOM_MONSTER_L6 = 0x1000000A3,
  OBJECT_RANDOM_MONSTER_L7 = 0x1000000A4,
  OBJECT_TERRAIN_BRUSH2 = 0x1000000A5,
  OBJECT_TERRAIN_BUSH2 = 0x1000000A6,
  OBJECT_TERRAIN_CACTUS2 = 0x1000000A7,
  OBJECT_TERRAIN_CANYON2 = 0x1000000A8,
  OBJECT_TERRAIN_CRATER2 = 0x1000000A9,
  OBJECT_TERRAIN_DEAD_VEGETATION2 = 0x1000000AA,
  OBJECT_TERRAIN_FLOWERS2 = 0x1000000AB,
  OBJECT_TERRAIN_FROZEN_LAKE2 = 0x1000000AC,
  OBJECT_TERRAIN_HEDGE2 = 0x1000000AD,
  OBJECT_TERRAIN_HILL2 = 0x1000000AE,
  OBJECT_TERRAIN_HOLE2 = 0x1000000AF,
  OBJECT_TERRAIN_KELP2 = 0x1000000B0,
  OBJECT_TERRAIN_LAKE2 = 0x1000000B1,
  OBJECT_TERRAIN_LAVA_FLOW2 = 0x1000000B2,
  OBJECT_TERRAIN_LAVA_LAKE2 = 0x1000000B3,
  OBJECT_TERRAIN_MUSHROOMS2 = 0x1000000B4,
  OBJECT_TERRAIN_LOG2 = 0x1000000B5,
  OBJECT_TERRAIN_MANDRAKE2 = 0x1000000B6,
  OBJECT_TERRAIN_MOSS2 = 0x1000000B7,
  OBJECT_TERRAIN_MOUND2 = 0x1000000B8,
  OBJECT_TERRAIN_MOUNTAIN2 = 0x1000000B9,
  OBJECT_TERRAIN_OAK_TREES2 = 0x1000000BA,
  OBJECT_TERRAIN_OUTCROPPING2 = 0x1000000BB,
  OBJECT_TERRAIN_PINE_TREES2 = 0x1000000BC,
  OBJECT_TERRAIN_PLANT2 = 0x1000000BD,
  OBJECT_TERRAIN_RIVER_DELTA2 = 0x1000000BE,
  OBJECT_TERRAIN_ROCK2 = 0x1000000BF,
  OBJECT_TERRAIN_SAND_DUNE2 = 0x1000000C0,
  OBJECT_TERRAIN_SAND_PIT2 = 0x1000000C1,
  OBJECT_TERRAIN_SHRUB2 = 0x1000000C2,
  OBJECT_TERRAIN_SKULL2 = 0x1000000C3,
  OBJECT_TERRAIN_STALAGMITE2 = 0x1000000C4,
  OBJECT_TERRAIN_STUMP2 = 0x1000000C5,
  OBJECT_TERRAIN_TAR_PIT2 = 0x1000000C6,
  OBJECT_TERRAIN_TREES2 = 0x1000000C7,
  OBJECT_TERRAIN_VINE2 = 0x1000000C8,
  OBJECT_TERRAIN_VOLCANIC_TENT2 = 0x1000000C9,
  OBJECT_TERRAIN_VOLCANO2 = 0x1000000CA,
  OBJECT_TERRAIN_WILLOW_TREES2 = 0x1000000CB,
  OBJECT_TERRAIN_YUCCA_TREES2 = 0x1000000CC,
  OBJECT_TERRAIN_REEF2 = 0x1000000CD,
  OBJECT_TERRAIN_DESERT_HILLS = 0x1000000CE,
  OBJECT_TERRAIN_DIRT_HILLS = 0x1000000CF,
  OBJECT_TERRAIN_GRASS_HILLS = 0x1000000D0,
  OBJECT_TERRAIN_ROUGH_HILLS = 0x1000000D1,
  OBJECT_TERRAIN_SUBTERRANEAN_ROCKS = 0x1000000D2,
  OBJECT_TERRAIN_SWAMP_FOLIAGE = 0x1000000D3,
  OBJECT_BORDER_GATE = 0x1000000D4,
  OBJECT_HERO_PLACEHOLDER = 0x1000000D6,
  OBJECT_FREELANCERS_GUILD = 0x2000000D5,
  OBJECT_QUEST_GUARD = 0x2000000D7,
  OBJECT_RANDOM_DWELLING = 0x2000000D8,
  OBJECT_RANDOM_DWELLING_LVL = 0x2000000D9,
  OBJECT_RANDOM_DWELLING_FACTION = 0x2000000DA,
  OBJECT_GARRISON2 = 0x2000000DB,
  OBJECT_ABANDONED_MINE = 0x2000000DC,
  OBJECT_TRADING_POST_SNOW = 0x2000000DD,
  OBJECT_CLOVER_FIELD = 0x2000000DE,
  OBJECT_CURSED_GROUND2 = 0x2000000DF,
  OBJECT_EVIL_FOG = 0x2000000E0,
  OBJECT_FAVORABLE_WINDS = 0x2000000E1,
  OBJECT_FIERY_FIELDS = 0x2000000E2,
  OBJECT_HOLY_GROUNDS = 0x2000000E3,
  OBJECT_LUCID_POOLS = 0x2000000E4,
  OBJECT_MAGIC_CLOUDS = 0x2000000E5,
  OBJECT_MAGIC_PLAINS2 = 0x2000000E6,
  OBJECT_ROCKLANDS = 0x2000000E7,
  OBJECT_INVALID = 0x2FFFFFFFF,
  K_NUM_OBJECTS_ROE = 0x3000000A5,
  K_NUM_OBJECTS_AB = 0x3000000DE,
  K_NUM_OBJECTS_SOD = 0x3000000E8,
};

struct type_obscuring_object
{
  int16 mapX;
  int16 mapY;
  int16 mapZ;
  bool valid;
  type_point obscured_location;
  byte aligned1;
  TAdventureObjectType type;
  bool was_trigger;
  int8[3] aligned2;
  ExtraInfoUnion extra_info;
};

enum THeroClass
{
  eClassKnight = 0x0,
  eClassCleric = 0x1,
  eClassRanger = 0x2,
  eClassDruid = 0x3,
  eClassAlchemist = 0x4,
  eClassWizard = 0x5,
  eClassPagan = 0x6,
  eClassHeretic = 0x7,
  eClassDeathKnight = 0x8,
  eClassNecromancer = 0x9,
  eClassOverlord = 0xA,
  eClassWarlock = 0xB,
  eClassBarbarian = 0xC,
  eClassBattleMage = 0xD,
  eClassBeastmaster = 0xE,
  eClassWitch = 0xF,
  eClassPlanesWalker = 0x10,
  eClassElementalist = 0x11,
  kNumHeroClasses = 0x12,
};

enum THeroID
{
  HERO_NONE = 0xFFFFFFFF,
  HERO_MOST_POWERFUL = 0x1FFFFFFFD,
  HERO_ORRIN = 0x200000000,
  HERO_VALESKA = 0x200000001,
  HERO_EDRIC = 0x200000002,
  HERO_SYLVIA = 0x200000003,
  HERO_LORD_HAART = 0x200000004,
  HERO_SORSHA = 0x200000005,
  HERO_CHRISTIAN = 0x200000006,
  HERO_TYRIS = 0x200000007,
  HERO_RION = 0x200000008,
  HERO_ADELA = 0x200000009,
  HERO_CUTHBERT = 0x20000000A,
  HERO_ADELAIDE = 0x20000000B,
  HERO_INGHAM = 0x20000000C,
  HERO_SANYA = 0x20000000D,
  HERO_LOYNIS = 0x20000000E,
  HERO_CAITLIN = 0x20000000F,
  HERO_MEPHALA = 0x200000010,
  HERO_UFRETIN = 0x200000011,
  HERO_JENOVA = 0x200000012,
  HERO_RYLAND = 0x200000013,
  HERO_THORGRIM = 0x200000014,
  HERO_IVOR = 0x200000015,
  HERO_CLANCY = 0x200000016,
  HERO_KYRRE = 0x200000017,
  HERO_CORONIUS = 0x200000018,
  HERO_ULAND = 0x200000019,
  HERO_ELLESHAR = 0x20000001A,
  HERO_GEM = 0x20000001B,
  HERO_MALCOM = 0x20000001C,
  HERO_MELODIA = 0x20000001D,
  HERO_ALAGAR = 0x20000001E,
  HERO_AERIS = 0x20000001F,
  HERO_PIQUEDRAM = 0x200000020,
  HERO_THANE = 0x200000021,
  HERO_JOSEPHINE = 0x200000022,
  HERO_NEELA = 0x200000023,
  HERO_TOROSAR = 0x200000024,
  HERO_FAFNER = 0x200000025,
  HERO_RISSA = 0x200000026,
  HERO_IONA = 0x200000027,
  HERO_ASTRAL = 0x200000028,
  HERO_HALON = 0x200000029,
  HERO_SERENA = 0x20000002A,
  HERO_DAREMYTH = 0x20000002B,
  HERO_THEODORUS = 0x20000002C,
  HERO_SOLMYR = 0x20000002D,
  HERO_CYRA = 0x20000002E,
  HERO_AINE = 0x20000002F,
  HERO_FIONA = 0x200000030,
  HERO_RASHKA = 0x200000031,
  HERO_MARIUS = 0x200000032,
  HERO_IGNATIUS = 0x200000033,
  HERO_OCTAVIA = 0x200000034,
  HERO_CALH = 0x200000035,
  HERO_PYRE = 0x200000036,
  HERO_NYMUS = 0x200000037,
  HERO_AYDEN = 0x200000038,
  HERO_XYRON = 0x200000039,
  HERO_AXSIS = 0x20000003A,
  HERO_OLEMA = 0x20000003B,
  HERO_CALID = 0x20000003C,
  HERO_ASH = 0x20000003D,
  HERO_ZYDAR = 0x20000003E,
  HERO_XARFAX = 0x20000003F,
  HERO_STRAKER = 0x200000040,
  HERO_VOKIAL = 0x200000041,
  HERO_MOANDOR = 0x200000042,
  HERO_CHARNA = 0x200000043,
  HERO_TAMIKA = 0x200000044,
  HERO_ISRA = 0x200000045,
  HERO_CLAVIUS = 0x200000046,
  HERO_GALTHRAN = 0x200000047,
  HERO_SEPTIENNA = 0x200000048,
  HERO_AISLINN = 0x200000049,
  HERO_SANDRO = 0x20000004A,
  HERO_NIMBUS = 0x20000004B,
  HERO_THANT = 0x20000004C,
  HERO_XSI = 0x20000004D,
  HERO_VIDOMINA = 0x20000004E,
  HERO_NAGASH = 0x20000004F,
  HERO_LORELEI = 0x200000050,
  HERO_ARLACH = 0x200000051,
  HERO_DACE = 0x200000052,
  HERO_AJIT = 0x200000053,
  HERO_DAMACON = 0x200000054,
  HERO_GUNNAR = 0x200000055,
  HERO_SYNCA = 0x200000056,
  HERO_SHAKTI = 0x200000057,
  HERO_ALAMAR = 0x200000058,
  HERO_JAEGAR = 0x200000059,
  HERO_MALEKITH = 0x20000005A,
  HERO_JEDDITE = 0x20000005B,
  HERO_GEON = 0x20000005C,
  HERO_DEEMER = 0x20000005D,
  HERO_SEPHINROTH = 0x20000005E,
  HERO_DARKSTORN = 0x20000005F,
  HERO_YOG = 0x200000060,
  HERO_GURNISSON = 0x200000061,
  HERO_JABARKAS = 0x200000062,
  HERO_SHIVA = 0x200000063,
  HERO_GRETCHIN = 0x200000064,
  HERO_KRELLION = 0x200000065,
  HERO_CRAG_HACK = 0x200000066,
  HERO_TYRAXOR = 0x200000067,
  HERO_GIRD = 0x200000068,
  HERO_VEY = 0x200000069,
  HERO_DESSA = 0x20000006A,
  HERO_TEREK = 0x20000006B,
  HERO_ZUBIN = 0x20000006C,
  HERO_GUNDULA = 0x20000006D,
  HERO_ORIS = 0x20000006E,
  HERO_SAURUG = 0x20000006F,
  HERO_BRON = 0x200000070,
  HERO_DRAKON = 0x200000071,
  HERO_WYSTAN = 0x200000072,
  HERO_TAZAR = 0x200000073,
  HERO_ALKIN = 0x200000074,
  HERO_KORBAC = 0x200000075,
  HERO_GERWULF = 0x200000076,
  HERO_BROGHILD = 0x200000077,
  HERO_MIRLANDA = 0x200000078,
  HERO_ROSIC = 0x200000079,
  HERO_VOY = 0x20000007A,
  HERO_VERDISH = 0x20000007B,
  HERO_MERIST = 0x20000007C,
  HERO_STYG = 0x20000007D,
  HERO_ANDRA = 0x20000007E,
  HERO_TIVA = 0x20000007F,
  HERO_PASIS = 0x200000080,
  HERO_THUNAR = 0x200000081,
  HERO_IGNISSA = 0x200000082,
  HERO_LACUS = 0x200000083,
  HERO_MONERE = 0x200000084,
  HERO_ERDAMON = 0x200000085,
  HERO_FIUR = 0x200000086,
  HERO_KALT = 0x200000087,
  HERO_LUNA = 0x200000088,
  HERO_BRISSA = 0x200000089,
  HERO_CIELE = 0x20000008A,
  HERO_LABETHA = 0x20000008B,
  HERO_INTEUS = 0x20000008C,
  HERO_AENAIN = 0x20000008D,
  HERO_GELARE = 0x20000008E,
  HERO_GRINDAN = 0x20000008F,
  HERO_SIR_MULLICH = 0x200000090,
  HERO_ADRIENNE = 0x200000091,
  HERO_CATHERINE = 0x200000092,
  HERO_DRACON = 0x200000093,
  HERO_GELU = 0x200000094,
  HERO_KILGOR = 0x200000095,
  HERO_LORD_HAART_LICH = 0x200000096,
  HERO_MUTARE = 0x200000097,
  HERO_ROLAND = 0x200000098,
  HERO_MUTARE_DRAKE = 0x200000099,
  HERO_BORAGUS = 0x20000009A,
  HERO_XERON = 0x20000009B,
  MAX_HEROES_SOD = 0x20000009C,
  MAX_HEROES_AB = 0x300000092,
  MAX_HEROES_ROE = 0x400000080,
  MAX_HEROES = 0x40000009C,
};

struct std::bitset_48_
{
  int[2] bitset_array;
};

enum TArtifact
{
  ARTIFACT_NONE = 0xFFFFFFFF,
  ARTIFACT_SPELLBOOK = 0x100000000,
  ARTIFACT_SPELL_SCROLL = 0x100000001,
  ARTIFACT_GRAIL = 0x200000000,
  ARTIFACT_CATAPULT = 0x200000003,
  ARTIFACT_BALLISTA = 0x200000004,
  ARTIFACT_AMMO_CART = 0x200000005,
  ARTIFACT_FIRST_AID_TENT = 0x200000006,
  ARTIFACT_CENTAUR_AXE = 0x200000007,
  ARTIFACT_BLACKSHARD_OF_THE_DEAD_KNIGHT = 0x200000008,
  ARTIFACT_GREATER_GNOLLS_FLAIL = 0x200000009,
  ARTIFACT_OGRES_CLUB_OF_HAVOC = 0x20000000A,
  ARTIFACT_SWORD_OF_HELLFIRE = 0x20000000B,
  ARTIFACT_TITANS_GLADIUS = 0x20000000C,
  ARTIFACT_SHIELD_OF_THE_DWARVEN_LORDS = 0x20000000D,
  ARTIFACT_SHIELD_OF_THE_YAWNING_DEAD = 0x20000000E,
  ARTIFACT_BUCKLER_OF_THE_GNOLL_KING = 0x20000000F,
  ARTIFACT_TARG_OF_THE_RAMPAGING_OGRE = 0x200000010,
  ARTIFACT_SHIELD_OF_THE_DAMNED = 0x200000011,
  ARTIFACT_SENTINELS_SHIELD = 0x200000012,
  ARTIFACT_HELM_OF_THE_ALABASTER_UNICORN = 0x200000013,
  ARTIFACT_SKULL_HELMET = 0x200000014,
  ARTIFACT_HELM_OF_CHAOS = 0x200000015,
  ARTIFACT_CROWN_OF_THE_SUPREME_MAGI = 0x200000016,
  ARTIFACT_HELLSTORM_HELMET = 0x200000017,
  ARTIFACT_THUNDER_HELMET = 0x200000018,
  ARTIFACT_BREASTPLATE_OF_PETRIFIED_WOOD = 0x200000019,
  ARTIFACT_RIB_CAGE = 0x20000001A,
  ARTIFACT_SCALES_OF_THE_GREATER_BASILISK = 0x20000001B,
  ARTIFACT_TUNIC_OF_THE_CYCLOPS_KING = 0x20000001C,
  ARTIFACT_BREASTPLATE_OF_BRIMSTONE = 0x20000001D,
  ARTIFACT_TITANS_CUIRASS = 0x20000001E,
  ARTIFACT_ARMOR_OF_WONDER = 0x20000001F,
  ARTIFACT_SANDALS_OF_THE_SAINT = 0x200000020,
  ARTIFACT_CELESTIAL_NECKLACE_OF_BLISS = 0x200000021,
  ARTIFACT_LIONS_SHIELD_OF_COURAGE = 0x200000022,
  ARTIFACT_SWORD_OF_JUDGEMENT = 0x200000023,
  ARTIFACT_HELM_OF_HEAVENLY_ENLIGHTENMENT = 0x200000024,
  ARTIFACT_QUIET_EYE_OF_THE_DRAGON = 0x200000025,
  ARTIFACT_RED_DRAGON_FLAME_TONGUE = 0x200000026,
  ARTIFACT_DRAGON_SCALE_SHIELD = 0x200000027,
  ARTIFACT_DRAGON_SCALE_ARMOR = 0x200000028,
  ARTIFACT_DRAGONBONE_GREAVES = 0x200000029,
  ARTIFACT_DRAGON_WING_TABARD = 0x20000002A,
  ARTIFACT_NECKLACE_OF_DRAGONTEETH = 0x20000002B,
  ARTIFACT_CROWN_OF_DRAGONTOOTH = 0x20000002C,
  ARTIFACT_STILL_EYE_OF_THE_DRAGON = 0x20000002D,
  ARTIFACT_CLOVER_OF_FORTUNE = 0x20000002E,
  ARTIFACT_CARDS_OF_PROPHECY = 0x20000002F,
  ARTIFACT_LADYBIRD_OF_LUCK = 0x200000030,
  ARTIFACT_BADGE_OF_COURAGE = 0x200000031,
  ARTIFACT_CREST_OF_VALOR = 0x200000032,
  ARTIFACT_GLYPH_OF_GALLANTRY = 0x200000033,
  ARTIFACT_SPECULUM = 0x200000034,
  ARTIFACT_SPYGLASS = 0x200000035,
  ARTIFACT_AMULET_OF_THE_UNDERTAKER = 0x200000036,
  ARTIFACT_VAMPIRES_COWL = 0x200000037,
  ARTIFACT_DEAD_MANS_BOOTS = 0x200000038,
  ARTIFACT_GARNITURE_OF_INTERFERENCE = 0x200000039,
  ARTIFACT_SURCOAT_OF_COUNTERPOISE = 0x20000003A,
  ARTIFACT_BOOTS_OF_POLARITY = 0x20000003B,
  ARTIFACT_BOW_OF_ELVEN_CHERRYWOOD = 0x20000003C,
  ARTIFACT_BOWSTRING_OF_THE_UNICORNS_MANE = 0x20000003D,
  ARTIFACT_ANGEL_FEATHER_ARROWS = 0x20000003E,
  ARTIFACT_BIRD_OF_PERCEPTION = 0x20000003F,
  ARTIFACT_STOIC_WATCHMAN = 0x200000040,
  ARTIFACT_EMBLEM_OF_COGNIZANCE = 0x200000041,
  ARTIFACT_STATESMANS_MEDAL = 0x200000042,
  ARTIFACT_DIPLOMATS_RING = 0x200000043,
  ARTIFACT_AMBASSADORS_SASH = 0x200000044,
  ARTIFACT_RING_OF_THE_WAYFARER = 0x200000045,
  ARTIFACT_EQUESTRIANS_GLOVES = 0x200000046,
  ARTIFACT_NECKLACE_OF_OCEAN_GUIDANCE = 0x200000047,
  ARTIFACT_ANGEL_WINGS = 0x200000048,
  ARTIFACT_CHARM_OF_MANA = 0x200000049,
  ARTIFACT_TALISMAN_OF_MANA = 0x20000004A,
  ARTIFACT_MYSTIC_ORB_OF_MANA = 0x20000004B,
  ARTIFACT_COLLAR_OF_CONJURING = 0x20000004C,
  ARTIFACT_RING_OF_CONJURING = 0x20000004D,
  ARTIFACT_CAPE_OF_CONJURING = 0x20000004E,
  ARTIFACT_ORB_OF_THE_FIRMAMENT = 0x20000004F,
  ARTIFACT_ORB_OF_SILT = 0x200000050,
  ARTIFACT_ORB_OF_TEMPESTUOUS_FIRE = 0x200000051,
  ARTIFACT_ORB_OF_DRIVING_RAIN = 0x200000052,
  ARTIFACT_RECANTERS_CLOAK = 0x200000053,
  ARTIFACT_SPIRIT_OF_OPPRESSION = 0x200000054,
  ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR = 0x200000055,
  ARTIFACT_TOME_OF_FIRE_MAGIC = 0x200000056,
  ARTIFACT_TOME_OF_AIR_MAGIC = 0x200000057,
  ARTIFACT_TOME_OF_WATER_MAGIC = 0x200000058,
  ARTIFACT_TOME_OF_EARTH_MAGIC = 0x200000059,
  ARTIFACT_BOOTS_OF_LEVITATION = 0x20000005A,
  ARTIFACT_GOLDEN_BOW = 0x20000005B,
  ARTIFACT_SPHERE_OF_PERMANENCE = 0x20000005C,
  ARTIFACT_ORB_OF_VULNERABILITY = 0x20000005D,
  ARTIFACT_RING_OF_VITALITY = 0x20000005E,
  ARTIFACT_RING_OF_LIFE = 0x20000005F,
  ARTIFACT_VIAL_OF_LIFEBLOOD = 0x200000060,
  ARTIFACT_NECKLACE_OF_SWIFTNESS = 0x200000061,
  ARTIFACT_BOOTS_OF_SPEED = 0x200000062,
  ARTIFACT_CAPE_OF_VELOCITY = 0x200000063,
  ARTIFACT_PENDANT_OF_DISPASSION = 0x200000064,
  ARTIFACT_PENDANT_OF_SECOND_SIGHT = 0x200000065,
  ARTIFACT_PENDANT_OF_HOLINESS = 0x200000066,
  ARTIFACT_PENDANT_OF_LIFE = 0x200000067,
  ARTIFACT_PENDANT_OF_DEATH = 0x200000068,
  ARTIFACT_PENDANT_OF_FREE_WILL = 0x200000069,
  ARTIFACT_PENDANT_OF_NEGATIVITY = 0x20000006A,
  ARTIFACT_PENDANT_OF_TOTAL_RECALL = 0x20000006B,
  ARTIFACT_PENDANT_OF_COURAGE = 0x20000006C,
  ARTIFACT_EVERFLOWING_CRYSTAL_CLOAK = 0x20000006D,
  ARTIFACT_RING_OF_INFINITE_GEMS = 0x20000006E,
  ARTIFACT_EVERPOURING_VIAL_OF_MERCURY = 0x20000006F,
  ARTIFACT_INEXHAUSTIBLE_CART_OF_ORE = 0x200000070,
  ARTIFACT_EVERSMOKING_RING_OF_SULFUR = 0x200000071,
  ARTIFACT_INEXHAUSTIBLE_CART_OF_LUMBER = 0x200000072,
  ARTIFACT_ENDLESS_SACK_OF_GOLD = 0x200000073,
  ARTIFACT_ENDLESS_BAG_OF_GOLD = 0x200000074,
  ARTIFACT_ENDLESS_PURSE_OF_GOLD = 0x200000075,
  ARTIFACT_LEGS_OF_LEGION = 0x200000076,
  ARTIFACT_LOINS_OF_LEGION = 0x200000077,
  ARTIFACT_TORSO_OF_LEGION = 0x200000078,
  ARTIFACT_ARMS_OF_LEGION = 0x200000079,
  ARTIFACT_HEAD_OF_LEGION = 0x20000007A,
  ARTIFACT_SEA_CAPTAINS_HAT = 0x20000007B,
  ARTIFACT_SPELLBINDERS_HAT = 0x20000007C,
  ARTIFACT_SHACKLES_OF_WAR = 0x20000007D,
  ARTIFACT_ORB_OF_INHIBITION = 0x20000007E,
  ARTIFACT_VIAL_OF_DRAGON_BLOOD = 0x20000007F,
  ARTIFACT_ARMAGEDDONS_BLADE = 0x200000080,
  ARTIFACT_ANGELIC_ALLIANCE = 0x200000081,
  ARTIFACT_CLOAK_OF_THE_UNDEAD_KING = 0x200000082,
  ARTIFACT_ELIXIR_OF_LIFE = 0x200000083,
  ARTIFACT_ARMOR_OF_THE_DAMNED = 0x200000084,
  ARTIFACT_STATUE_OF_LEGION = 0x200000085,
  ARTIFACT_POWER_OF_THE_DRAGON_FATHER = 0x200000086,
  ARTIFACT_TITANS_THUNDER = 0x200000087,
  ARTIFACT_ADMIRALS_HAT = 0x200000088,
  ARTIFACT_BOW_OF_THE_SHARPSHOOTER = 0x200000089,
  ARTIFACT_WIZARDS_WELL = 0x20000008A,
  ARTIFACT_RING_OF_THE_MAGI = 0x20000008B,
  ARTIFACT_CORNUCOPIA = 0x20000008C,
  MAX_ARTIFACTS_ROE = 0x30000007F,
  MAX_ARTIFACTS_AB = 0x300000081,
  MAX_ARTIFACTS_SOD = 0x300000090,
  MAX_ARTIFACTS = 0x300000090,
  MAX_COMBO_ARTIFACTS = 0x40000000C,
};

struct type_artifact
{
  TArtifact type;
  SpellID spell;
};

struct hero : type_obscuring_object
{
  int16 mana;
  THeroID id;
  int order;
  int8 playerOwner;
  int8[13] name;
  THeroClass hero_class;
  uchar portrait;
  int targetX;
  int targetY;
  int16 targetZ;
  int16 last_magic_school_level;
  int16 target_distance;
  bool target_is_critical;
  uchar patrolX;
  uchar patrolY;
  uchar patrolRadius;
  uchar facing;
  uchar formation;
  int maxMobility;
  int currMobility;
  int experience;
  int16 Level;
  int LearningStoneFlags;
  int DefenseTowerFlags;
  int GardenOfRevelationFlags;
  int MercCampFlags;
  int StarAxisFlags;
  int TreeOfKnowledgeFlags;
  int LibraryFlags;
  int ArenaFlags;
  int MagicSchoolFlags;
  int WarSchoolFlags;
  int UniversityFlags;
  int Shrine1Flags;
  int Shrine2Flags;
  int Shrine3Flags;
  uchar iLevelSeed;
  uchar lastWisdom;
  armyGroup heroArmy;
  int8[28] SSLevel;
  uchar[28] SSOrder;
  int numSSs;
  uint flags;
  float turnExperienceToRVRatio;
  int8 dWalkSpellsCast;
  TSkillMastery disguiseLevel;
  TSkillMastery flightLevel;
  TSkillMastery waterWalkPower;
  int8 moraleBonus;
  int8 luckBonus;
  bool IsSleeping;
  int bounty;
  std::bitset_48_ TownSpecialGrantedMask;
  int visionsPower;
  type_artifact[19] equipped;
  int8[15] blockedSlots;
  type_artifact[64] backpack;
  int8 backpack_count;
  int sex;
  bool bio_customized;
  std::string bio;
  bool[70] in_spellbook;
  bool[70] available_spells;
  int8[4] stats;
  int aggression;
  int value_of_power;
  int value_of_duration;
  int value_of_knowledge;
  int value_of_spring;
  int value_of_well;
};

enum Formation
{
  GroupedFormation = 0x1,
  PlacementFormation = 0x2,
};

enum TArtifactSlot
{
  SLOT_HEAD = 0x0,
  SLOT_SHOULDERS = 0x1,
  SLOT_NECK = 0x2,
  SLOT_RIGHT_HAND = 0x3,
  SLOT_LEFT_HAND = 0x4,
  SLOT_TORSO = 0x5,
  SLOT_RIGHT_RING = 0x6,
  SLOT_LEFT_RING = 0x7,
  SLOT_FEET = 0x8,
  SLOT_MISC1 = 0x9,
  SLOT_MISC2 = 0xA,
  SLOT_MISC3 = 0xB,
  SLOT_MISC4 = 0xC,
  SLOT_BALLISTA = 0xD,
  SLOT_AMMO_CART = 0xE,
  SLOT_FIRST_AID_TENT = 0xF,
  SLOT_CATAPULT = 0x10,
  SLOT_SPELLBOOK = 0x11,
  SLOT_MISC5 = 0x12,
  MAX_ARTIFACT_SLOTS_ROE = 0x12,
  MAX_ARTIFACT_SLOTS_AB = 0x12,
  MAX_ARTIFACT_SLOTS_SOD = 0x13,
  MAX_ARTIFACT_SLOTS = 0x13,
  MAX_BACKPACK_SLOTS = 0x40,
};

enum TSecondarySkill
{
  SKILL_PATHFINDING = 0x0,
  SKILL_ARCHERY = 0x1,
  SKILL_LOGISTICS = 0x2,
  SKILL_SCOUTING = 0x3,
  SKILL_DIPLOMACY = 0x4,
  SKILL_NAVIGATION = 0x5,
  SKILL_LEADERSHIP = 0x6,
  SKILL_WISDOM = 0x7,
  SKILL_MYSTICISM = 0x8,
  SKILL_LUCK = 0x9,
  SKILL_BALLISTICS = 0xA,
  SKILL_EAGLE_EYE = 0xB,
  SKILL_NECROMANCY = 0xC,
  SKILL_ESTATES = 0xD,
  SKILL_FIRE_MAGIC = 0xE,
  SKILL_AIR_MAGIC = 0xF,
  SKILL_WATER_MAGIC = 0x10,
  SKILL_EARTH_MAGIC = 0x11,
  SKILL_SCHOLAR = 0x12,
  SKILL_TACTICS = 0x13,
  SKILL_ARTILLERY = 0x14,
  SKILL_LEARNING = 0x15,
  SKILL_OFFENSE = 0x16,
  SKILL_ARMORER = 0x17,
  SKILL_INTELLIGENCE = 0x18,
  SKILL_SORCERY = 0x19,
  SKILL_RESISTANCE = 0x1A,
  SKILL_FIRST_AID = 0x1B,
  SKILL_NONE = 0xFFFFFFFF,
  MAX_SECONDARY_SKILLS = 0x10000001C,
  SKILL_NUM = 0x10000001C,
  MAX_SKILL_PER_HERO = 0x200000008,
};

struct baseManager::vftable_t
{
  int (__thiscall *)(baseManager* Open, int this);
  void (__thiscall *)(baseManager* Open);
  int (__thiscall *)(baseManager* Open, message* this);
};

struct baseManager
{
  baseManager::vftable_t* vftable;
  baseManager* nextManager;
  baseManager* prevManager;
  int id;
  int priority;
  int8[32] cMgrName;
  int status;
};

struct heroWindowManager : baseManager
{
  DialogReturnType dialogReturn;
  int lastHover;
  Bitmap16Bit* screenBitmap;
  int colorCyclingOn;
  bool isWaitingForFadeIn;
  Bitmap16Bit* bmpFizzleSource;
  heroWindow* activeWindow;
  heroWindow* lastActive;
  heroWindow* headWindow;
  heroWindow* tailWindow;
};

struct message
{
  message::ECommandType command;
  int subType;
  int itemId;
  message::EQualifiers qualifier;
  int mouseX;
  int mouseY;
  int extra;
  heroWindow* window;
};

struct File::vftable_t
{
  bool (__thiscall *)(File* Close);
  bool (__thiscall *)(File* Close, int8* this, FileMode Open);
  bool (__thiscall *)(File* Close);
  DWORD (__thiscall *)(File* Close, void* this, DWORD Open);
  DWORD (__thiscall *)(File* Close, void* this, DWORD Open);
  DWORD (__thiscall *)(File* Close, DWORD this, DWORD Open);
  DWORD (__thiscall *)(File* Close);
  DWORD (__thiscall *)(File* Close);
  DWORD (__thiscall *)(File* Close, int this);
  DWORD (__thiscall *)(File* Close);
  DWORD (__thiscall *)(File* Close);
};

struct File
{
  File::vftable_t* vftable;
  void* m_hFile;
  int8[256] sLastError;
  bool open;
};

struct CNetMsgHandlerPause : CNetMsgHandler
{
  CNetMsgHandler* m_pNetMsgHandlerSave;
};

struct soundManager : baseManager
{
  HANDLE mssHandle;
  HDIGDRIVER driver;
  HSAMPLE* samples;
  HSAMPLE*[14] samples_array;
  int sampleNum;
  int currentTerrainMusic;
  bool32 playSounds;
  bool32 bChangeSounds;
  bool32 MP3Playing;
  CRITICAL_SECTION section_sound_call;
  CRITICAL_SECTION section_MP3_change;
  CRITICAL_SECTION section_MP3_name_change;
};

struct CDiffMaker
{
  BYTE* m_oldData;
  BYTE* m_newData;
  int m_oldSize;
  int m_newSize;
};

struct Buffer { };

struct TDialogBox : heroWindow
{
  int beginID;
  int endID;
};

enum widget::EStatusFlags
{
  WIDGET_STATUS_MASK = 0xFFFF,
  WIDGET_SELECTED = 0x100000001,
  WIDGET_ACTIVE = 0x100000002,
  WIDGET_DRAWN = 0x100000004,
  WIDGET_DIMMED = 0x100000008,
  WIDGET_HIGHLIGHTED = 0x100000010,
  WIDGET_DISABLED = 0x100000020,
  WIDGET_DIMMED_NODRAW = 0x100001000,
  WIDGET_ASLEEP = 0x100002000,
  WIDGET_UPDATE = 0x100004000,
};

enum widget::ECommands
{
  WIDGET_ACTIVATE = 0x1,
  WIDGET_DRAW = 0x2,
  WIDGET_SET_TEXT = 0x3,
  WIDGET_SET_ICON_FRAME = 0x4,
  WIDGET_SET_STATUS = 0x5,
  WIDGET_CLEAR_STATUS = 0x6,
  WIDGET_GET_TEXT = 0x7,
  WIDGET_SET_ICON_COLOR = 0x8,
  WIDGET_SET_COLOR = 0x8,
  WIDGET_SET_ICON_NAME = 0x9,
  WIDGET_SET_PALETTE = 0xA,
  WIDGET_SET_IMAGE = 0xB,
  WIDGET_SET_ICON_SEQUENCE = 0xC,
  WIDGET_SET_PLAYER_PALETTE_COLORS = 0xD,
  WIDGET_SET_SLIDER_STATE = 0x31,
  WIDGET_SET_SLIDER_RESOLUTION = 0x32,
  WIDGET_SET_TEXT_LEN = 0x33,
  WIDGET_SET_X = 0x34,
  WIDGET_SET_Y = 0x35,
  WIDGET_SET_ITEM = 0x36,
  WIDGET_GET_ITEM = 0x37,
  WIDGET_ADD_ITEM = 0x38,
  WIDGET_CHANGE_ITEM = 0x39,
  WIDGET_DELETE_ITEM = 0x3A,
  WIDGET_DELETE_ALL_ITEMS = 0x3B,
  WIDGET_SET_WIDTH = 0x3D,
  WIDGET_SET_HEIGHT = 0x3E,
  WIDGET_SET_COLORIZE = 0x3F,
  WIDGET_SET_FOCUS = 0x40,
};

enum widget::EReturnCodes
{
  WIDGET_END_DIALOG = 0xA,
  WIDGET_SELECT = 0xC,
  WIDGET_DESELECT = 0xD,
  WIDGET_RIGHT_SELECT = 0xE,
};

enum widget::EQualifiers
{
  WIDGET_SINGLE_CLICK = 0x1,
  WIDGET_DOUBLE_CLICK = 0x2,
};

enum widget::ETypes
{
  NULL_WIDGET = 0x0,
  BORDER = 0x1,
  BUTTON = 0x2,
  TEXT_BUTTON = 0x3,
  DRAGBAR = 0x4,
  TEXT_WIDGET = 0x8,
  ICON_WIDGET = 0x10,
  ICON_WIDGET_XC_YBM2 = 0x11,
  ICON_WIDGET_CREATURE = 0x12,
  UPDATE_WIDGET = 0x20,
  DIMMER_WIDGET = 0x40,
  MONO_WIDGET = 0x80,
  TEXT_ENTRY_WIDGET = 0x100,
  COLORED_BORDER = 0x400,
  BITMAP_BORDER = 0x800,
  HIT_SELECT_BUTTON = 0x1000,
  REL_VERIFY_BUTTON = 0x2000,
  USER = 0x4000,
};

struct CHeroWindowEx : heroWindow
{
  int m_lastIMHoverID;
};

struct THelpText
{
  int8* Rollover;
  int8* RightClick;
};

struct std::bitset_28_
{
  uint _bits;
};

struct std::vector_bool_
{
  int8 allocator;
  bool* first;
  bool* last;
  bool* end;
};

struct TPickANumber
{
  int Low;
  int Numbersleft;
  std::vector_bool_ Available;
};

struct combatManager : baseManager
{
  CNetMsgHandlerPause* netMsgHandlerPause;
  int iNextAction;
  int iNextActionExtra;
  int iNextActionGridIndex;
  int iNextActionGridIndex2;
  bool[187] iLastDrawGridShade;
  bool[187] iCurDrawGridShade;
  bool unknown_;
  hexcell[187] cell;
  int combatTerrain;
  int combatFringe;
  int iCombatCycleType;
  int iElevationOverlay;
  int iDoorStatus;
  bool bMoatOn;
  bool moatIsWide;
  Bitmap16Bit* SaveScreenPreGrid;
  Bitmap16Bit* SaveScreenPostGrid;
  Bitmap16Bit* combatMouseBackground;
  int bBackgroundDrawn;
  NewmapCell* EventCell;
  EMagicTerrain magic_terrain;
  bool OnBoats;
  bool OnBeach;
  town* combatTown;
  hero*[2] Heroes;
  int[2] iSideSpellPower;
  bool[2] PlayDoh;
  bool[2] PlayYeah;
  bool[2] DohPlayedThisRound;
  bool[2] YeahPlayedThisRound;
  int[2] cmbtHeroFrameType;
  int[2] cmbtHeroFrameIndex;
  int[2] cmbtHeroDataSet;
  ulong[2] cmbtHeroLastFidgetTime;
  CSprite*[2] cmbtHero;
  CSprite*[2] cmbtHeroFlag;
  int[2] cmbtHeroFlagFrame;
  SLimitData[2] sCmbtHeroLimitData;
  SLimitData[2] sCmbtHeroFlagLimitData;
  std::vector[2] EagleEyeSpellLearned;
  uchar[20][2] ArmyEffected;
  bool[2] IsHuman;
  bool[2] IsLocalHuman;
  int[2] iPlayer;
  bool[2] bArtifactCast;
  int[2] bSpellsCast;
  int[2] numArmies;
  armyGroup*[2] ArmyGroups;
  army[21][2] Armies;
  bool aligned;
  bool unknown;
  int[2] turnSinceLastEnchanter;
  unsigned int[2] nativeTerrain;
  bool[2] SummonedElemental;
  int SideRetreated;
  int currArmyGroup;
  int currArmyIndex;
  int currControl;
  int autoCombatOn;
  army* currTroop;
  uchar selectorOn;
  int selectorIndex;
  int highlighterOn;
  int highlighterIndex;
  int lastMoveToIndex;
  int lastCommand;
  int combatCommand;
  CSprite* CurLoadedSpellIcon;
  int CurLoadedSpellEffect;
  int CurSpellEffectFrame;
  TFortificationLevel fortificationLevel;
  int bBattleOver;
  TCombatWindow* mainWindow;
  int bCombatShowIt;
  iconWidget*[25] iconWidgetWL;
  textWidget*[25] textWidgetWL;
  type_combat_cursor[12] attack_cursor;
  int[12] attack_hex;
  type_combat_cursor last_attack_cursor;
  int iTtlCombatDirections;
  int iBackgroundFrame;
  bool[20][2] bCreatureIsDead;
  bool bSomeCreaturesVanish;
  int8* cBkgName;
  combatManager::adjacency_array[187] AdjacentIndex;
  uchar SaveBiggestExtent;
  int LimitToExtent;
  int ComputeExtentOnly;
  SLimitData Extent;
  int winner;
  int SkeletonsCreated;
  TCreatureType skeleton_type;
  Bitmap816* NumberWindow;
  std::vector_combatManager::TObstacle_ Obstacles;
  bool InPlacementPhase;
  int turn_number;
  int BattleTacticsAdvantage;
  bool DebugNoSpellLimit;
  bool DebugShowHiddenObjects;
  bool DebugShowBlockedHexes;
  combatManager::TArcher[3] Archers;
  bool in_second_phase;
  int OriginalAttackSkill;
  int OriginalDefenseSkill;
  int OriginalPowerSkill;
  int original_mana;
  Bitmap816*[5][18] wallImages;
  int[18] wallLevel;
  int[18] wall_frame;
  type_point map_point;
  Bitmap816* CombatCellGrid;
  Bitmap816* CombatCellShaded;
  int ObstacleAnimationFrame;
  bool[20][2] bCreatureEffect;
  bool[2] bHeroEffect;
  bool[2] bFlagEffect;
  bool[3] bArcherEffect;
  bool any_action_taken;
  byte[1] unknownlast;
  bool[187] creaturePath;
};

struct TPickRandomTownName : TPickANumber { };

struct TTextResource : resource
{
  std::vector_char_ptr_ Text;
  int8* Data;
};

struct std::vector_char_ptr_
{
  int8 allocator;
  int8** first;
  int8** last;
  int8** end;
};

struct TNormalDialogInfo
{
  std::string dialog_text;
  int x;
  int y;
  int width;
  int height;
  int text_widget_x;
  int text_widget_y;
  int text_widget_width;
  int text_widget_height;
  bool text_expansion;
  type_dialog_icon[8] icons;
  EMBType iMBType;
  int iSpecial;
  int timeout;
};

struct playerData
{
  int8 color;
  int8 numHeroes;
  byte[2] align1;
  int currHero;
  THeroID[8] heroes;
  THeroID[2] recruits;
  uchar startingNumHeroes;
  byte[3] align2;
  int personality;
  int8 extraPuzzlePieces;
  type_point puzzle_guess;
  int8 iDeathCountDown;
  int8 numTowns;
  int8 currTown;
  int8[72] towns;
  bool placement_help_enabled;
  byte[3] align3;
  std::vector shipyards;
  int[7] resources;
  std::bitset_32_ MysticalGardenFlags;
  std::bitset_32_ MagicSpringFlags;
  std::bitset_32_ DeadGuyFlags;
  std::bitset_32_ LeanToFlags;
  ulong dpid;
  int8[21] cName;
  bool isLocal;
  bool isHuman;
  byte[1] align4;
  int quickCombat;
  std::bitset_12_ builtArtifacts;
  AI ai;
};

struct NewmapCell : ExtraInfoUnion
{
  int8 GroundSet;
  int8 GroundIndex;
  int8 RiverSet;
  int8 RiverIndex;
  int8 RoadSet;
  int8 RoadIndex;
  ushort align1;
  unsigned int16 GroundFlippedHorizontal : 1;
  unsigned int16 GroundFlippedVertical : 1;
  unsigned int16 RiverFlippedHorizontal : 1;
  unsigned int16 RiverFlippedVertical : 1;
  unsigned int16 RoadFlippedHorizontal : 1;
  unsigned int16 RoadFlippedVertical : 1;
  unsigned int16 Passable : 1;
  unsigned int16 Animated : 1;
  unsigned int16 IsBlocked : 1;
  unsigned int16 IsBeachBorder : 1;
  unsigned int16 unused_bit : 1;
  unsigned int16 can_build_ship : 1;
  unsigned int16 is_trigger : 1;
  std::vector_NewmapCell__TObjectCell_ ObjectCellList;
  TAdventureObjectType type;
  int16 objectIndex;
  int16 object_type_index;
};

struct TAdvMenu : CAdvPopup
{
  int CheatKey;
  uchar nextPlayer;
};

struct CNetPlayerInfo
{
  ulong dpid;
  int8[24] cName;
  int version;
};

struct CDPlayHeroes : CDPlayLobby
{
  CDPlayMsg dpMsg;
  std::deque_CNetMsg_ptr_ msgQueue;
  int8[80] sLocalIPAddress;
  ulong confirmId;
  ulong currMessageId;
  CNetMsgHandler* m_pNetMsgHandler;
};

struct mouseManager : baseManager
{
  int bNoChangePointer;
  tagRECT LastDraw;
  mouseManager::EPointerSet Set;
  int Frame;
  CSprite* Sprite;
  int ImageX;
  int ImageY;
  int DisableCount;
  bool SystemPointerIsOn;
  int iHideCount;
  POINT cursorPos;
  int Busy;
  CRITICAL_SECTION CriticalSection;
};

struct CAdvMgrNetMsgHandler : CNetMsgHandler { };

struct mapCellArtifact
{
  unsigned int32 price : 4;
  unsigned int32 guard : 8;
  unsigned int32 resource_price : 4;
  unsigned int32 guard_qty : 2;
  unsigned int32 custom : 12;
  unsigned int32  : 1;
};

struct mapCellDefaultObject
{
  unsigned int id;
};

struct mapCellCampfire
{
  unsigned int32 amount : 4;
  unsigned int32 resource : 16;
  unsigned int32  : 12;
};

struct mapCellCorpse
{
  unsigned int32 id : 5;
  unsigned int32 artifact : 1;
  int32 has_treasure : 10;
  unsigned int32  : 1;
  unsigned int32  : 15;
};

struct mapCellCreatureBank
{
  unsigned int32 visited_bits : 5;
  unsigned int32 index : 8;
  unsigned int32 empty : 12;
  unsigned int32  : 1;
  unsigned int32  : 6;
};

struct mapCellEvent
{
  unsigned int32 index : 10;
  unsigned int32 allow_player : 8;
  unsigned int32 allow_computer : 1;
  unsigned int32 cancel_after_visit : 1;
  unsigned int32  : 12;
};

enum mapCellFlotsam::EMapCellFlotSam
{
  FLOTSAM_EMPTY = 0x0,
  FLOTSAM_WOOD5 = 0x1,
  FLOTSAM_WOOD5_GOLD200 = 0x2,
  FLOTSAM_WOOD10_GOLD500 = 0x3,
};

struct mapCellFlotsam
{
  uint type;
};

struct mapCellFountainFortune
{
  unsigned int32 visited_bits : 5;
  unsigned int32 luck_bonus : 8;
  int32  : 4;
  unsigned int32  : 15;
};

struct mapCellLeanTo
{
  unsigned int32 id : 5;
  unsigned int32 amount : 1;
  unsigned int32 resource : 4;
  unsigned int32  : 4;
  unsigned int32  : 18;
};

struct mapCellMagicShrine
{
  unsigned int32 visited_bits : 5;
  unsigned int32 spell : 8;
  unsigned int32  : 10;
  unsigned int32  : 9;
};

struct mapCellMagicSpring
{
  unsigned int32 id : 5;
  unsigned int32 full : 1;
  unsigned int32  : 1;
  unsigned int32  : 25;
};

struct mapCellMonster
{
  unsigned int32 qty : 12;
  unsigned int32 disposition : 5;
  unsigned int32 never_flee : 1;
  unsigned int32 dont_grow : 1;
  unsigned int32 index : 8;
  unsigned int32 growth_remainder : 4;
  unsigned int32 custom : 1;
};

struct mapCellMysticGarden
{
  unsigned int32 id : 5;
  unsigned int32 resource : 1;
  unsigned int32 full : 4;
  unsigned int32  : 1;
  unsigned int32  : 21;
};

struct mapCellPandorasBox
{
  unsigned int32 index : 10;
  unsigned int32  : 22;
};

struct mapCellPyramid
{
  unsigned int32 guarded : 1;
  unsigned int32 visited_bits : 4;
  unsigned int32 spell : 8;
  unsigned int32  : 8;
  unsigned int32  : 11;
};

struct mapCellRefugeeCamp
{
  int amount;
};

struct mapCellResource
{
  unsigned int32 value : 19;
  unsigned int32 setupIndex : 12;
  unsigned int32 hasSetup : 1;
};

struct mapCellScholar
{
  unsigned int32 type : 3;
  unsigned int32 PSkill : 3;
  unsigned int32 SSkill : 7;
  unsigned int32 spell : 10;
  unsigned int32  : 9;
};

struct mapCellScroll
{
  unsigned int32 spell : 8;
  unsigned int32 index : 11;
  unsigned int32 custom : 12;
  unsigned int32  : 1;
};

struct mapCellSeaChest
{
  unsigned int32 reward : 3;
  int32 artifact : 10;
  unsigned int32  : 19;
};

struct mapCellShipwreckSurvivor
{
  TArtifact artifact;
};

struct mapCellShipyard
{
  unsigned int32 owner : 8;
  unsigned int32 boatX : 8;
  unsigned int32 boatY : 8;
  unsigned int32  : 8;
};

struct mapCellTreasureChest
{
  int32 artifact : 10;
  unsigned int32 is_artifact : 1;
  unsigned int32 gold_amount : 4;
  unsigned int32  : 17;
};

struct mapCellTreeOfKnowledge
{
  unsigned int32 id : 5;
  unsigned int32 visited_bits : 8;
  unsigned int32 price : 2;
  unsigned int32  : 17;
};

struct mapCellUniversity
{
  unsigned int32 visited_bits : 5;
  unsigned int32 index : 8;
  unsigned int32  : 12;
  unsigned int32  : 7;
};

struct mapCellWagon
{
  unsigned int32 resource_amount : 5;
  unsigned int32 visited_bits : 8;
  unsigned int32 full : 1;
  unsigned int32 has_artifact : 1;
  int32 artifact : 10;
  unsigned int32 resource : 4;
  unsigned int32  : 3;
};

struct mapCellWarriorsTomb
{
  unsigned int32 full : 1;
  unsigned int32 visited_bits : 4;
  unsigned int32 artifact : 8;
  int32  : 10;
  unsigned int32  : 9;
};

struct mapCellWaterMill
{
  unsigned int32 amount : 5;
  unsigned int32 visited_bits : 8;
  unsigned int32  : 19;
};

struct mapCellWindMill
{
  unsigned int32 resource : 4;
  unsigned int32 visited_bits : 1;
  unsigned int32 amount : 8;
  unsigned int32  : 4;
  unsigned int32  : 15;
};

struct mapCellWitchHut
{
  unsigned int32 visited_bits : 5;
  unsigned int32 skill : 8;
  int32  : 7;
  unsigned int32  : 12;
};

union ExtraInfoUnion::InfoUnion
{
  uint extraInfo;
  mapCellArtifact Artifact;
  mapCellDefaultObject BlackMarket;
  mapCellDefaultObject Boat;
  mapCellCampfire campfire;
  mapCellCorpse corpse;
  mapCellCreatureBank creatureBank;
  mapCellEvent event;
  mapCellFlotsam flotsam;
  mapCellFountainFortune fountainFortune;
  mapCellDefaultObject garrison;
  mapCellDefaultObject generator;
  mapCellDefaultObject hero;
  mapCellLeanTo leanTo;
  mapCellDefaultObject learningStone;
  mapCellDefaultObject lighthouse;
  mapCellMagicShrine magicShrine;
  mapCellMagicSpring magicSpring;
  mapCellDefaultObject mine;
  mapCellDefaultObject monolith;
  mapCellMonster wanderingCreature;
  mapCellMysticGarden mysticGarden;
  mapCellDefaultObject obelisk;
  mapCellDefaultObject oceanBottle;
  mapCellPandorasBox pandorasBox;
  mapCellDefaultObject prison;
  mapCellPyramid pyramid;
  mapCellDefaultObject questGuard;
  mapCellRefugeeCamp refugeeCamp;
  mapCellResource resource;
  mapCellScholar scholar;
  mapCellScroll spellScroll;
  mapCellSeaChest seaChest;
  mapCellDefaultObject seerHut;
  mapCellShipwreckSurvivor shipwreckSurvivor;
  mapCellShipyard shipyard;
  mapCellDefaultObject signPost;
  mapCellDefaultObject town;
  mapCellTreasureChest treasureChest;
  mapCellTreeOfKnowledge treeKnowledge;
  mapCellUniversity university;
  mapCellWagon wagon;
  mapCellWarriorsTomb warriorsTomb;
  mapCellWaterMill watermill;
  mapCellWindMill windmill;
  mapCellWitchHut witchHut;
};

union ExtraInfoUnion::$78D74CA2AC055E83C754EB0E8B29499D
{
  uint extraInfo;
  mapCellArtifact Artifact;
  mapCellDefaultObject BlackMarket;
  mapCellDefaultObject Boat;
  mapCellCampfire campfire;
  mapCellCorpse corpse;
  mapCellCreatureBank creatureBank;
  mapCellEvent event;
  mapCellFlotsam flotsam;
  mapCellFountainFortune fountainFortune;
  mapCellDefaultObject garrison;
  mapCellDefaultObject generator;
  mapCellDefaultObject hero;
  mapCellLeanTo leanTo;
  mapCellDefaultObject learningStone;
  mapCellDefaultObject lighthouse;
  mapCellMagicShrine magicShrine;
  mapCellMagicSpring magicSpring;
  mapCellDefaultObject mine;
  mapCellDefaultObject monolith;
  mapCellMonster wanderingCreature;
  mapCellMysticGarden mysticGarden;
  mapCellDefaultObject obelisk;
  mapCellDefaultObject oceanBottle;
  mapCellPandorasBox pandorasBox;
  mapCellDefaultObject prison;
  mapCellPyramid pyramid;
  mapCellDefaultObject questGuard;
  mapCellRefugeeCamp refugeeCamp;
  mapCellResource resource;
  mapCellScholar scholar;
  mapCellScroll spellScroll;
  mapCellSeaChest seaChest;
  mapCellDefaultObject seerHut;
  mapCellShipwreckSurvivor shipwreckSurvivor;
  mapCellShipyard shipyard;
  mapCellDefaultObject signPost;
  mapCellDefaultObject town;
  mapCellTreasureChest treasureChest;
  mapCellTreeOfKnowledge treeKnowledge;
  mapCellUniversity university;
  mapCellWagon wagon;
  mapCellWarriorsTomb warriorsTomb;
  mapCellWaterMill watermill;
  mapCellWindMill windmill;
  mapCellWitchHut witchHut;
};

struct ExtraInfoUnion
{
  ExtraInfoUnion::$78D74CA2AC055E83C754EB0E8B29499D ;
};

enum EResourceType
{
  RType_invalid = 0xFFFFFFFF,
  RType_misc = 0x100000000,
  RType_null = 0x100000000,
  RType_data = 0x100000001,
  RType_text = 0x100000002,
  RType_bitmap = 0x100000010,
  RType_bitmap8 = 0x100000010,
  RType_bitmap24 = 0x100000011,
  RType_bitmap16 = 0x100000012,
  RType_bitmap565 = 0x100000013,
  RType_bitmap555 = 0x100000014,
  RType_bitmap1555 = 0x100000015,
  RType_sfx = 0x100000020,
  RType_midi = 0x100000030,
  RType_sprite = 0x100000040,
  RType_spritedef = 0x100000041,
  RType_creature = 0x100000042,
  RType_advobj = 0x100000043,
  RType_hero = 0x100000044,
  RType_tileset = 0x100000045,
  RType_pointer = 0x100000046,
  RType_interface = 0x100000047,
  RType_spriteframe = 0x100000048,
  RType_combat_hero = 0x100000049,
  RType_advmask = 0x10000004F,
  RType_font = 0x100000050,
  RType_palette = 0x100000060,
};

enum heroWindow::TAttribute
{
  NORMAL = 0x0,
  BACKDROP = 0x1,
  SAVEBACK = 0x2,
  DIALOG = 0x4,
  NOSAVEBACK = 0x8,
  DROP_SHADOW = 0x10,
};

enum heroWindow::TStatus
{
  NO_STATUS = 0x0,
  VISIBLE = 0x1,
  CORRUPT = 0x2,
};

enum TViewWorldWindow::EOtherWidgetIDs
{
};

enum EGameResource
{
  const_no_resource = 0xFFFFFFFF,
  WOOD = 0x100000000,
  MERCURY = 0x100000001,
  ORE = 0x100000002,
  SULFUR = 0x100000003,
  CRYSTAL = 0x100000004,
  GEMS = 0x100000005,
  GOLD = 0x100000006,
  ABANDONED = 0x100000007,
  RES_ARTIFACT = 0x100000008,
  RES_SPELL = 0x100000009,
  RES_COLOR = 0x10000000A,
  RES_GOOD_LUCK = 0x10000000B,
  RES_NEUTRAL_LUCK = 0x10000000C,
  RES_BAD_LUCK = 0x10000000D,
  RES_GOOD_MORALE = 0x10000000E,
  RES_NEUTRAL_MORALE = 0x10000000F,
  RES_BAD_MORALE = 0x100000010,
  RES_EXPERIENCE = 0x100000011,
  RES_HERO = 0x100000012,
  RES_ARTIFACT_W_TEXT = 0x100000013,
  RES_SECONDARY_SKILL = 0x100000014,
  RES_MONSTER = 0x100000015,
  RES_BUILDING_TT_0 = 0x100000016,
  RES_BUILDING_TT_1 = 0x100000017,
  RES_BUILDING_TT_2 = 0x100000018,
  RES_BUILDING_TT_3 = 0x100000019,
  RES_BUILDING_TT_4 = 0x10000001A,
  RES_BUILDING_TT_5 = 0x10000001B,
  RES_BUILDING_TT_6 = 0x10000001C,
  RES_BUILDING_TT_7 = 0x10000001D,
  RES_PRIMARY_SKILL_ATTACK = 0x10000001E,
  RES_PRIMARY_SKILL_DEFENSE = 0x10000001F,
  RES_PRIMARY_SKILL_POWER = 0x100000020,
  RES_PRIMARY_SKILL_KNOWLEDGE = 0x100000021,
  RES_MANA = 0x100000022,
  RES_SMALL_GOLD = 0x100000023,
};

enum TTerrainType
{
  eTerrainNone = 0xFFFFFFFF,
  eTerrainDirt = 0x100000000,
  eTerrainSand = 0x100000001,
  eTerrainGrass = 0x100000002,
  eTerrainSnow = 0x100000003,
  eTerrainSwamp = 0x100000004,
  eTerrainRough = 0x100000005,
  eTerrainSubterranean = 0x100000006,
  eTerrainLava = 0x100000007,
  eTerrainWater = 0x100000008,
  eTerrainRock = 0x100000009,
  kNumTerrainTypes = 0x10000000A,
  eTerrainBeach = 0x10000000C,
  eTerrainMagicPlains = 0x10000000D,
  eTerrainCursedGround = 0x10000000E,
};

enum TQuickTownWindow::TViewLevel
{
  ViewArmyTypes = 0x1,
  ViewArmySizes = 0x2,
};

enum TQuickTownWindow::EWidgetIDs
{
  HALL_LEVEL_ID = 0x7D3,
  CASTLE_LEVEL_ID = 0x7D4,
  TYPE_LEVEL_NAME_ID = 0x7D5,
  GOLD_PER_DAY_ID = 0x7D6,
  RESOURCE_BONUS_ID = 0x7D7,
  GARRISON_HERO_ID = 0x7D8,
};

enum TQuickHeroWindow::TViewLevel
{
  ViewSome = 0x1,
};

enum TQuickHeroWindow::EOtherWidgetIDs
{
  POWER_ID = 0x7D5,
  KNOWLEDGE_ID = 0x7D6,
  CLASS_ID = 0x7D7,
  ARMY_1_SPRITE_ID = 0x7DB,
  ARMY_1_QUANTITY_ID = 0x7DC,
  ARMY_2_SPRITE_ID = 0x7DD,
  ARMY_2_QUANTITY_ID = 0x7DE,
  ARMY_3_SPRITE_ID = 0x7DF,
  ARMY_3_QUANTITY_ID = 0x7E0,
  ARMY_4_SPRITE_ID = 0x7E1,
  ARMY_4_QUANTITY_ID = 0x7E2,
  ARMY_5_SPRITE_ID = 0x7E3,
  ARMY_5_QUANTITY_ID = 0x7E4,
  ARMY_6_SPRITE_ID = 0x7E5,
  ARMY_6_QUANTITY_ID = 0x7E6,
  ARMY_7_SPRITE_ID = 0x7E7,
  ARMY_7_QUANTITY_ID = 0x7E8,
};

enum type_building_id
{
  MAGE_GUILD_ID = 0x0,
  MAGE_GUILD2_ID = 0x1,
  MAGE_GUILD3_ID = 0x2,
  MAGE_GUILD4_ID = 0x3,
  MAGE_GUILD5_ID = 0x4,
  TAVERN_ID = 0x5,
  DOCK_ID = 0x6,
  CASTLE_FORT_ID = 0x7,
  CASTLE_CITADEL_ID = 0x8,
  CASTLE_CASTLE_ID = 0x9,
  HALL_VILLAGE_ID = 0xA,
  HALL_TOWN_ID = 0xB,
  HALL_CITY_ID = 0xC,
  HALL_CAPITOL_ID = 0xD,
  MARKETPLACE_ID = 0xE,
  MARKETPLACE_SILO_ID = 0xF,
  BLACKSMITH_ID = 0x10,
  SPECIAL_BUILDING_ID = 0x11,
  HORDE_ID = 0x12,
  HORDE_UPG_ID = 0x13,
  DOCK_WITH_BOAT_ID = 0x14,
  EXTRA_0_ID = 0x15,
  EXTRA_1_ID = 0x16,
  EXTRA_2_ID = 0x17,
  HORDE_2_ID = 0x18,
  HORDE_2_UPG_ID = 0x19,
  HOLY_GRAIL_ID = 0x1A,
  EXTRA_3_ID = 0x1B,
  EXTRA_4_ID = 0x1C,
  EXTRA_5_ID = 0x1D,
  DWELLING_0_ID = 0x1E,
  DWELLING_1_ID = 0x1F,
  DWELLING_2_ID = 0x20,
  DWELLING_3_ID = 0x21,
  DWELLING_4_ID = 0x22,
  DWELLING_5_ID = 0x23,
  DWELLING_6_ID = 0x24,
  DWELLING_0_UPG_ID = 0x25,
  DWELLING_1_UPG_ID = 0x26,
  DWELLING_2_UPG_ID = 0x27,
  DWELLING_3_UPG_ID = 0x28,
  DWELLING_4_UPG_ID = 0x29,
  DWELLING_5_UPG_ID = 0x2A,
  DWELLING_6_UPG_ID = 0x2B,
  MAX_BUILDING_TYPE = 0x2C,
  const_village_hall = 0x10000000A,
  const_town_hall = 0x10000000B,
  const_city_hall = 0x10000000C,
  const_capitol_hall = 0x10000000D,
  const_fort = 0x200000007,
  const_citadel = 0x200000008,
  const_castle = 0x200000009,
  const_tavern = 0x300000005,
  const_blacksmith = 0x300000010,
  const_market = 0x40000000E,
  const_resource_silo = 0x40000000F,
  const_mage_guild_1 = 0x500000000,
  const_mage_guild_2 = 0x500000001,
  const_mage_guild_3 = 0x500000002,
  const_mage_guild_4 = 0x500000003,
  const_mage_guild_5 = 0x500000004,
  const_shipyard = 0x500000006,
  LIGHTHOUSE_ID = 0x500000011,
  STABLES_ID = 0x500000015,
  TAVERN_UPG_ID = 0x500000016,
  const_colossus = 0x50000001A,
  const_lighthouse = 0x600000011,
  const_brotherhood_of_the_sword = 0x600000016,
  const_horse_stable = 0x700000015,
  const_pikeman_generator = 0x70000001E,
  const_halberdier_generator = 0x700000025,
  const_light_crossbowman_generator = 0x80000001F,
  const_heavy_crossbowman_generator = 0x800000026,
  const_griffin_generator = 0x900000020,
  const_royal_griffin_generator = 0x900000027,
  const_griffin_horde = 0xA00000012,
  const_royal_griffin_horde = 0xA00000013,
  const_swordsman_generator = 0xA00000021,
  const_crusader_generator = 0xA00000028,
  const_monk_generator = 0xB00000022,
  const_zealot_generator = 0xB00000029,
  const_cavalier_generator = 0xC00000023,
  const_champion_generator = 0xC0000002A,
  const_angel_generator = 0xD00000024,
  const_archangel_generator = 0xD0000002B,
  const_storytelling_flora = 0xE0000001A,
  const_mystic_garden = 0xF00000011,
  const_rainbow = 0xF00000015,
  const_treasury = 0xF00000016,
  const_house_1 = 0xF00000017,
  const_house_2 = 0xF0000001B,
  const_house_3 = 0xF0000001C,
  const_house_4 = 0xF0000001D,
  const_centaur_generator = 0xF0000001E,
  const_elite_centaur_generator = 0xF00000025,
  const_dwarf_generator = 0x100000001F,
  const_battle_dwarf_generator = 0x1000000026,
  const_dwarf_horde = 0x1100000012,
  const_battle_dwarf_horde = 0x1100000013,
  const_wood_elf_generator = 0x1100000020,
  const_grand_elf_generator = 0x1100000027,
  const_pegasus_generator = 0x1200000021,
  const_silver_pegasus_generator = 0x1200000028,
  const_treefolk_generator = 0x1300000022,
  const_briar_treefolk_generator = 0x1300000029,
  const_treefolk_horde = 0x1400000018,
  const_briar_treefolk_horde = 0x1400000019,
  const_unicorn_generator = 0x1400000023,
  const_war_unicorn_generator = 0x140000002A,
  const_green_dragon_generator = 0x1500000024,
  const_gold_dragon_generator = 0x150000002B,
  ARTIFACT_MERCHANTS_ID = 0x1600000011,
  WATCHTOWER_ID = 0x1600000015,
  LIBRARY_ID = 0x1600000016,
  WALL_OF_GLYPHIC_KNOWLEDGE_ID = 0x1600000017,
  const_skyship = 0x160000001A,
  const_artifact_merchants = 0x1700000011,
  const_library = 0x1700000016,
  const_watchtower = 0x1800000015,
  const_wall_of_glyphic_knowledge = 0x1800000017,
  const_apprentice_gremlin_generator = 0x180000001E,
  const_master_gremlin_generator = 0x1800000025,
  const_stone_gargoyle_generator = 0x190000001F,
  const_obsidian_gargoyle_generator = 0x1900000026,
  const_gargoyle_horde = 0x1A00000012,
  const_obsidian_gargoyle_horde = 0x1A00000013,
  const_stone_golem_generator = 0x1A00000020,
  const_iron_golem_generator = 0x1A00000027,
  const_mage_generator = 0x1B00000021,
  const_archmage_generator = 0x1B00000028,
  const_genie_generator = 0x1C00000022,
  const_caliph_generator = 0x1C00000029,
  const_naga_sentinel_generator = 0x1D00000023,
  const_naga_guardian_generator = 0x1D0000002A,
  const_lesser_titan_generator = 0x1E00000024,
  const_greater_titan_generator = 0x1E0000002B,
  BRIMSTONE_STORMCLOUDS_ID = 0x1F00000015,
  CASTLE_GATE_ID = 0x1F00000016,
  ORDER_OF_PAIN_ID = 0x1F00000017,
  const_eternal_effigy = 0x1F0000001A,
  const_brimstone_stormclouds = 0x2000000015,
  const_castle_gate = 0x2000000016,
  const_order_of_pain = 0x2000000017,
  const_imp_generator = 0x200000001E,
  const_familiar_generator = 0x2000000025,
  const_imp_horde = 0x2100000012,
  const_familiar_horde = 0x2100000013,
  const_gog_generator = 0x210000001F,
  const_magog_generator = 0x2100000026,
  const_hellhound_generator = 0x2200000020,
  const_cerberus_generator = 0x2200000027,
  const_hellhound_horde = 0x2300000018,
  const_cerberus_horde = 0x2300000019,
  const_single_horned_demon_generator = 0x2300000021,
  const_dual_horned_demon_generator = 0x2300000028,
  const_pit_fiend_generator = 0x2400000022,
  const_pit_foe_generator = 0x2400000029,
  const_efreet_generator = 0x2500000023,
  const_efreet_sultan_generator = 0x250000002A,
  const_devil_generator = 0x2600000024,
  const_arch_devil_generator = 0x260000002B,
  const_king_of_terror = 0x270000001A,
  const_shroud_generator = 0x2800000011,
  const_necromancy_amplifier = 0x2800000015,
  const_skeleton_transformer = 0x2800000016,
  const_skeleton_generator = 0x280000001E,
  const_skeleton_warrior_generator = 0x2800000025,
  const_skeleton_horde = 0x2900000012,
  const_skeleton_warrior_horde = 0x2900000013,
  const_zombie_generator = 0x290000001F,
  const_zombie_lord_generator = 0x2900000026,
  const_wight_generator = 0x2A00000020,
  const_wraith_generator = 0x2A00000027,
  const_vampire_generator = 0x2B00000021,
  const_nosferatu_generator = 0x2B00000028,
  const_lich_generator = 0x2C00000022,
  const_power_lich_generator = 0x2C00000029,
  const_black_knight_generator = 0x2D00000023,
  const_black_lord_generator = 0x2D0000002A,
  const_bone_dragon_generator = 0x2E00000024,
  const_ghost_dragon_generator = 0x2E0000002B,
  const_river_of_blood = 0x2F0000001A,
  const_mana_vortex = 0x3000000015,
  const_portal_of_summoning = 0x3000000016,
  const_academy_of_battle_scholars = 0x3000000017,
  const_troglodyte_generator = 0x300000001E,
  const_infernal_troglodyte_generator = 0x3000000025,
  const_troglodyte_horde = 0x3100000012,
  const_infernal_troglodyte_horde = 0x3100000013,
  const_harpy_generator = 0x310000001F,
  const_harpy_hag_generator = 0x3100000026,
  const_beholder_generator = 0x3200000020,
  const_evil_eye_generator = 0x3200000027,
  const_medusa_generator = 0x3300000021,
  const_medusa_queen_generator = 0x3300000028,
  const_minotaur_generator = 0x3400000022,
  const_minotaur_king_generator = 0x3400000029,
  const_manticore_generator = 0x3500000023,
  const_scorpicore_generator = 0x350000002A,
  const_red_dragon_generator = 0x3600000024,
  const_black_dragon_generator = 0x360000002B,
  const_weapon_array = 0x370000001A,
  const_escape_tunnel = 0x3800000011,
  const_freelancers_guild = 0x3800000015,
  const_ballista_works = 0x3800000016,
  const_hall_of_valhalla = 0x3800000017,
  const_goblin_generator = 0x380000001E,
  const_hobgoblin_generator = 0x3800000025,
  const_goblin_horde = 0x3900000012,
  const_hobgoblin_horde = 0x3900000013,
  const_goblin_wolf_rider_generator = 0x390000001F,
  const_hobgoblin_wolf_rider_generator = 0x3900000026,
  const_orc_generator = 0x3A00000020,
  const_orc_chieftan_generator = 0x3A00000027,
  const_ogre_generator = 0x3B00000021,
  const_ogre_mage_generator = 0x3B00000028,
  const_roc_generator = 0x3C00000022,
  const_thunderbird_generator = 0x3C00000029,
  const_cyclops_generator = 0x3D00000023,
  const_cyclops_lord_generator = 0x3D0000002A,
  const_young_behemoth_generator = 0x3E00000024,
  const_ancient_behemoth_generator = 0x3E0000002B,
  DEFENSE_CAGE_ID = 0x3F00000011,
  SIEGE_DEFENSE_ID = 0x3F00000015,
  SIEGE_ATTACK_ID = 0x3F00000016,
  const_carnivorous_plant = 0x3F0000001A,
  const_blood_obelisk = 0x4000000015,
  const_glyphs_of_fear = 0x4000000016,
  const_defense_cage = 0x4100000011,
  const_gnoll_generator = 0x410000001E,
  const_gnoll_marauder_generator = 0x4100000025,
  const_gnoll_horde = 0x4200000012,
  const_gnoll_marauder_horde = 0x4200000013,
  const_primitive_lizardman_generator = 0x420000001F,
  const_advanced_lizardman_generator = 0x4200000026,
  const_dragonfly_generator = 0x4300000020,
  const_fire_dragonfly_generator = 0x4300000027,
  const_basilisk_generator = 0x4400000021,
  const_greater_basilisk_generator = 0x4400000028,
  const_copper_gorgon_generator = 0x4500000022,
  const_bronze_gorgon_generator = 0x4500000029,
  const_wyvern_generator = 0x4600000023,
  const_wyvern_monarch_generator = 0x460000002A,
  const_hydra_generator = 0x4700000024,
  const_chaos_hydra_generator = 0x470000002B,
};

enum TSpellbookWindow::EWidgetIDs
{
};

enum TSpellbookWindow::TSpellContext
{
  eContextInvalid = 0xFFFFFFFF,
  eContextCombat = 0x100000000,
  eContextAdventure = 0x100000001,
  eContextNeither = 0x100000002,
};

enum TSpellSchool
{
  const_invalid_school = 0x0,
  eSchoolAir = 0x1,
  eSchoolFire = 0x2,
  eSchoolWater = 0x4,
  eSchoolEarth = 0x8,
  eSchoolAll = 0xF,
  kNumSpellSchools = 0x100000004,
};

enum TSpellbookWindow::EOtherWidgetIDs
{
  SPELL_LEVEL_0_ID = 0xC9,
  SPELL_LEVEL_1_ID = 0xCA,
  SPELL_LEVEL_2_ID = 0xCB,
  SPELL_LEVEL_3_ID = 0xCC,
  SPELL_LEVEL_4_ID = 0xCD,
  SPELL_LEVEL_5_ID = 0xCE,
  SPELL_LEVEL_6_ID = 0xCF,
  SPELL_LEVEL_7_ID = 0xD0,
  SPELL_LEVEL_8_ID = 0xD1,
  SPELL_LEVEL_9_ID = 0xD2,
  SPELL_LEVEL_10_ID = 0xD3,
  SPELL_LEVEL_11_ID = 0xD4,
  SPELL_0_ID = 0xD5,
  SPELL_1_ID = 0xD6,
  SPELL_2_ID = 0xD7,
  SPELL_3_ID = 0xD8,
  SPELL_4_ID = 0xD9,
  SPELL_5_ID = 0xDA,
  SPELL_6_ID = 0xDB,
  SPELL_7_ID = 0xDC,
  SPELL_8_ID = 0xDD,
  SPELL_9_ID = 0xDE,
  SPELL_10_ID = 0xDF,
  SPELL_11_ID = 0xE0,
  SCHOOL_TABS_ID = 0xE1,
  AIR_SCHOOL_ID = 0xE2,
  FIRE_SCHOOL_ID = 0xE3,
  WATER_SCHOOL_ID = 0xE4,
  EARTH_SCHOOL_ID = 0xE5,
  ALL_SCHOOL_ID = 0xE6,
  COMBAT_SPELLS_ID = 0xE7,
  ADVENTURE_SPELLS_ID = 0xE8,
  SPELL_POINTS_ID = 0xE9,
  PREVIOUS_PAGE_ID = 0xEA,
  NEXT_PAGE_ID = 0xEB,
  SCHOOL_HEADING_ID = 0xEC,
  SPELL_0_NAME_ID = 0xED,
  SPELL_1_NAME_ID = 0xEE,
  SPELL_2_NAME_ID = 0xEF,
  SPELL_3_NAME_ID = 0xF0,
  SPELL_4_NAME_ID = 0xF1,
  SPELL_5_NAME_ID = 0xF2,
  SPELL_6_NAME_ID = 0xF3,
  SPELL_7_NAME_ID = 0xF4,
  SPELL_8_NAME_ID = 0xF5,
  SPELL_9_NAME_ID = 0xF6,
  SPELL_10_NAME_ID = 0xF7,
  SPELL_11_NAME_ID = 0xF8,
};

enum army::TSampleID
{
  WALK_SAMPLE = 0x0,
  ATTACK_SAMPLE = 0x1,
  WINCE_SAMPLE = 0x2,
  SHOOT_SAMPLE = 0x3,
  DIE_SAMPLE = 0x4,
  DEFEND_SAMPLE = 0x5,
  PRE_WALK_SAMPLE = 0x6,
  POST_WALK_SAMPLE = 0x7,
  MAX_SAMPLES = 0x8,
};

enum TWallTargetId
{
  const_no_wall_target = 0xFFFFFFFF,
  eTargetUpperTower = 0x100000000,
  eTargetUpperWall = 0x100000001,
  eTargetMidUpperWall = 0x100000002,
  eTargetGate = 0x100000003,
  eTargetMidLowerWall = 0x100000004,
  eTargetLowerWall = 0x100000005,
  eTargetLowerTower = 0x100000006,
  eTargetMainBuilding = 0x100000007,
  kNumWallTargets = 0x100000008,
  const_first_wall_target = 0x200000000,
};

enum ds_genericsample::play_state
{
  STOPPED = 0x0,
  PLAYING = 0x1,
  PAUSED = 0x2,
};

enum mouseManager::EPointerSet
{
  SAME_SET = 0xFFFFFFFF,
  INVALID_SET = 0xFFFFFFFF,
  DEFAULT_SET = 0x100000000,
  ADVENTURE_SET = 0x100000001,
  COMBAT_SET = 0x100000002,
  SPELL_SET = 0x100000003,
  ARTIFACT_SET = 0x100000004,
  MAX_POINTER_SETS = 0x100000005,
};

enum slider::EGraphics
{
  BROWN = 0x0,
  BLUE = 0x1,
};

enum type_search_type
{
  const_normal_search = 0x0,
  const_AI_treasure_search = 0x1,
  const_AI_allied_search = 0x2,
  const_AI_enemy_search = 0x3,
  const_AI_search = 0x4,
  const_AI_alternate_search = 0x5,
};

enum TViewArmyWindow::EWidgetIDs
{
  UPGRADE_ID = 0x12C,
};

enum TViewArmyWindow::EOtherWidgetIDs
{
  SPRITE_ID = 0xC9,
  SPRITE_BACKGROUND_ID = 0xCA,
  NAME_ID = 0xCB,
  NUMBER_ID = 0xCC,
  ATTACK_LABEL_ID = 0xCD,
  ATTACK_ID = 0xCE,
  DEFENSE_LABEL_ID = 0xCF,
  DEFENSE_ID = 0xD0,
  SHOTS_LABEL_ID = 0xD1,
  SHOTS_ID = 0xD2,
  DAMAGE_LABEL_ID = 0xD3,
  DAMAGE_ID = 0xD4,
  HEALTH_LABEL_ID = 0xD5,
  HEALTH_ID = 0xD6,
  HEALTH_REMAINING_LABEL_ID = 0xD7,
  HEALTH_REMAINING_ID = 0xD8,
  SPEED_LABEL_ID = 0xD9,
  SPEED_ID = 0xDA,
  MORALE_ID = 0xDB,
  LUCK_ID = 0xDC,
  AFFECTING_SPELLS_0_ID = 0xDD,
  AFFECTING_SPELLS_1_ID = 0xDE,
  AFFECTING_SPELLS_2_ID = 0xDF,
  OK_BORDER_ID = 0xE1,
};

enum eNetGameType
{
  MP_SINGLE = 0x0,
  MP_IPX = 0x1,
  MP_TCP = 0x2,
  MP_HOTSEAT = 0x3,
  MP_SERIAL = 0x4,
  MP_MODEM = 0x5,
};

enum CNetMsg::eRS_Messages
{
  RS_GAME_TRANSMIT_INIT = 0x3E8,
  RS_GAME_TRANSMIT_MAIN = 0x3E9,
  RS_GAME_TRANSMIT_REQ = 0x3EA,
  RS_GAME_TRANSMIT_END = 0x3EB,
  RS_CHAT_MSG = 0x3EC,
  RS_COMBAT_INIT = 0x3ED,
  RS_COMBAT_MAIN = 0x3EE,
  RS_COMBAT_CONTROL = 0x3EF,
  RS_COMBAT_END_PLACEMENT = 0x3F0,
  RS_COMBAT_TYPE = 0x3F1,
  RS_MAP_CHANGE = 0x3F2,
  RS_HERO_LEVEL_UPDATE = 0x3F3,
  RS_READY_TO_PLAY = 0x3F4,
  RS_ALL_READY_TO_PLAY = 0x3F5,
  RS_PLAYER_DROPPED = 0x3F6,
  RS_SET_AS_HOST = 0x3F7,
  RS_TURN_UPDATE = 0x3F8,
  RS_PLAYER_DROP_UPDATE = 0x3F9,
  RS_PLAYER_DEAD = 0x3FA,
  RS_PLAYER_WON = 0x3FB,
  RS_PLAYER_LOST = 0x3FC,
  RS_SET_VISIBILITY = 0x3FD,
  RS_RESET_VISIBILITY = 0x3FE,
  RS_GAME_HEADER_INFO = 0x3FF,
  RS_GAME_HEADER_INFO_INIT = 0x400,
  RS_GAME_HEADER_INFO_END = 0x401,
  RS_NEW_SETUP_INFO = 0x402,
  RS_SCROLL = 0x403,
  RS_NEW_MAP_HEADER_INFO = 0x404,
  RS_MAP_HEADER_REQUEST = 0x405,
  RS_MAP_FILE_NAME = 0x406,
  RS_SORT_MAPS = 0x407,
  RS_SET_FILTER = 0x408,
  RS_NEXT_SCREEN = 0x409,
  RS_PREV_SCREEN = 0x40A,
  RS_REQUEST_HERO_FACE = 0x40B,
  RS_REQUEST_HERO_FACE_REPLY = 0x40C,
  RS_SETAGR = 0x40D,
  RS_NEW_HOST = 0x40E,
  RS_UPDATE_PLAYER_POS = 0x40F,
  RS_NEW_PLAYER = 0x410,
  RS_REQ_HEADER_CONFIRM = 0x411,
  RS_HEADER_CONFIRM = 0x412,
  RS_CLICK = 0x413,
  RS_TOWN_UPDATE = 0x414,
  RS_LAUNCHING_GAME = 0x415,
  RS_BAD_VERSION = 0x416,
  RS_SETUP_PING = 0x417,
  RS_SETUP_PING_RESPONSE = 0x418,
  RS_MAP_CHANGE_START = 0x419,
  RS_MOVE_HERO = 0x41A,
  RS_TELEPORT_HERO = 0x41B,
  RS_CLAIM_MINE = 0x41C,
  RS_CLAIM_TOWN = 0x41D,
  RS_CLAIM_GENERATOR = 0x41E,
  RS_CLAIM_GARRISON = 0x41F,
  RS_CLAIM_SHIPYARD = 0x420,
  RS_BUILD_BOAT = 0x421,
  RS_ERASE_OBJECT = 0x422,
  RS_DEAD_HERO = 0x423,
  RS_RECRUIT_HERO = 0x424,
  RS_DEAD_PLAYER = 0x425,
  RS_HIDE_HERO = 0x426,
  RS_MAP_CHANGE_END = 0x427,
  RS_TRADE_REQUEST = 0x428,
  RS_TRADE_REQUEST_DONE = 0x429,
  RS_HERO_UPDATE = 0x42A,
  RS_ARTIFACT_DROP = 0x42B,
  RS_BACKPACK_DROP = 0x42C,
  RS_MONSTER_DROP = 0x42D,
  RS_GIVE_ME_STUFF = 0x42E,
  RS_PLAYER_ACTIVE = 0x42F,
  RS_PING = 0x430,
  RS_PING_RESPONSE = 0x431,
  RS_GIFT = 0x432,
  RS_GIFT_REQUEST = 0x433,
  RS_SESSION_LOST = 0x434,
  RS_TEAM_WON = 0x435,
  RS_NORMAL_WIN = 0x436,
  RS_DESTROY_PLAYER = 0x437,
  RS_GAME_TRANSMIT_ACK = 0x438,
  RS_GAME_XFER_CONFIRM_END = 0x439,
  RS_GENERATING_RANDOM_MAP = 0x43A,
  RS_REQUEST_RANDOM_MAPS_LIST_SIZE = 0x43B,
  RS_REQUEST_RANDOM_MAPS_LIST = 0x43C,
};

enum type_adventure_cursor
{
  ADV_ARROW_POINTER = 0x0,
  ADV_WAIT_POINTER = 0x1,
  ADV_HERO_INFO_POINTER = 0x2,
  ADV_TOWN_INFO_POINTER = 0x3,
  ADV_WALK_POINTER = 0x4,
  ADV_SWORD_POINTER = 0x5,
  ADV_BOAT_POINTER = 0x6,
  ADV_ANCHOR_POINTER = 0x7,
  ADV_EXCHANGE_POINTER = 0x8,
  ADV_EVENT_POINTER = 0x9,
  ADV_MULTI_TURN_OFFSET = 0x100000006,
  ADV_BOAT_EVENT_POINTER = 0x10000001C,
  ADV_SCROLL_POINTER = 0x100000020,
  ADV_SCROLL_NORTH = 0x100000020,
  ADV_SCROLL_NORTHEAST = 0x100000021,
  ADV_SCROLL_EAST = 0x100000022,
  ADV_SCROLL_SOUTHEAST = 0x100000023,
  ADV_SCROLL_SOUTH = 0x100000024,
  ADV_SCROLL_SOUTHWEST = 0x100000025,
  ADV_SCROLL_WEST = 0x100000026,
  ADV_SCROLL_NORTHWEST = 0x100000027,
  ADV_HIGHLIGHTED_POINTER = 0x100000028,
  ADV_DIMENSION_DOOR_POINTER = 0x100000029,
  ADV_SKUTTLE_BOAT_POINTER = 0x10000002A,
};

enum e_looping_sound_id
{
  invalid_sound = 0xFFFFFFFF,
  minotaur_generator_sound = 0x100000000,
  gorgon_generator_sound = 0x100000000,
  crossbowman_generator_sound = 0x100000001,
  lizardman_generator_sound = 0x100000001,
  arena_sound = 0x100000002,
  behemoth_generator_sound = 0x100000003,
  roc_generator_sound = 0x100000004,
  buoy_sound = 0x100000005,
  campfire_sound = 0x100000006,
  beholder_generator_sound = 0x100000007,
  cyclops_generator_sound = 0x100000007,
  demon_generator_sound = 0x100000007,
  troglodyte_generator_sound = 0x100000007,
  cyclops_bank_sound = 0x100000007,
  abandoned_mine_sound = 0x100000007,
  black_knight_generator_sound = 0x100000008,
  lich_generator_sound = 0x100000008,
  vampire_generator_sound = 0x100000008,
  wight_generator_sound = 0x100000008,
  sepulcher_sound = 0x100000008,
  zombie_generator_sound = 0x100000008,
  devil_generator_sound = 0x100000009,
  hell_hound_generator_sound = 0x10000000A,
  bone_dragon_generator_sound = 0x10000000B,
  green_dragon_generator_sound = 0x10000000B,
  red_dragon_generator_sound = 0x10000000B,
  dragon_city_sound = 0x10000000B,
  golem_factory_sound = 0x10000000C,
  siege_weapon_factory_sound = 0x10000000C,
  fountain_of_youth_sound = 0x10000000D,
  fire_elemental_generator_sound = 0x10000000E,
  imp_generator_sound = 0x10000000E,
  pit_fiend_generator_sound = 0x10000000E,
  imp_bank_sound = 0x10000000E,
  pillar_of_fire_sound = 0x10000000E,
  rally_flag_sound = 0x10000000F,
  fountain_of_fortune_sound = 0x100000010,
  water_elemental_generator_sound = 0x100000010,
  magic_spring_sound = 0x100000010,
  gem_mine_sound = 0x100000011,
  gremlin_generator_sound = 0x100000012,
  griffin_generator_sound = 0x100000013,
  gargoyle_generator_sound = 0x100000013,
  griffin_bank_sound = 0x100000013,
  harpy_generator_sound = 0x100000014,
  centaur_generator_sound = 0x100000015,
  stables_sound = 0x100000015,
  cavalier_generator_sound = 0x100000015,
  hydra_generator_sound = 0x100000016,
  training_grounds_sound = 0x100000017,
  dragonfly_generator_sound = 0x100000017,
  dragonfly_bank_sound = 0x100000017,
  wood_mine_sound = 0x100000018,
  shipyard_sound = 0x100000018,
  magic_plains_sound = 0x100000019,
  genie_generator_sound = 0x100000019,
  mage_generator_sound = 0x100000019,
  anti_magic_garrison_sound = 0x100000019,
  magic_school_sound = 0x100000019,
  black_market_sound = 0x10000001A,
  trading_post_sound = 0x10000001A,
  merc_camp_sound = 0x10000001B,
  refugee_camp_sound = 0x10000001B,
  water_wheel_sound = 0x10000001C,
  gold_mine_sound = 0x10000001D,
  lith_one_way_sound = 0x10000001E,
  lith_two_way_sound = 0x10000001F,
  monk_generator_sound = 0x100000020,
  basilisk_generator_sound = 0x100000021,
  wyvern_generator_sound = 0x100000021,
  orc_generator_sound = 0x100000022,
  gnoll_generator_sound = 0x100000022,
  pegasus_generator_sound = 0x100000023,
  pikeman_generator_sound = 0x100000024,
  angel_generator_sound = 0x100000025,
  sanctuary_sound = 0x100000025,
  temple_sound = 0x100000025,
  shrine_sound = 0x100000026,
  star_axis_sound = 0x100000027,
  mercury_mine_sound = 0x100000027,
  sulfur_mine_sound = 0x100000028,
  ore_mine_sound = 0x100000028,
  war_school_sound = 0x100000029,
  defense_tower_sound = 0x100000029,
  hill_fort_sound = 0x100000029,
  garrison_sound = 0x100000029,
  swordsman_generator_sound = 0x10000002A,
  titan_generator_sound = 0x10000002B,
  elemental_conflux_sound = 0x10000002B,
  unicorn_generator_sound = 0x10000002C,
  volcano_sound = 0x10000002D,
  air_elemental_generator_sound = 0x10000002E,
  crystal_mine_sound = 0x10000002F,
  cursed_ground_sound = 0x100000030,
  thieves_guild_sound = 0x100000031,
  dwarf_generator_sound = 0x100000032,
  dwarf_bank_sound = 0x100000032,
  earth_elemental_generator_sound = 0x100000033,
  elf_generator_sound = 0x100000034,
  faerie_ring_sound = 0x100000035,
  garden_of_revelation_sound = 0x100000036,
  treefolk_generator_sound = 0x100000036,
  underground_gate_sound = 0x100000037,
  goblin_generator_sound = 0x100000038,
  mystical_garden_sound = 0x100000039,
  manticore_generator_sound = 0x10000003A,
  medusa_generator_sound = 0x10000003B,
  medusa_bank_sound = 0x10000003B,
  naga_generator_sound = 0x10000003C,
  naga_bank_sound = 0x10000003C,
  ogre_generator_sound = 0x10000003D,
  siren_sound = 0x10000003E,
  skeleton_generator_sound = 0x10000003F,
  tavern_sound = 0x100000040,
  efreet_generator_sound = 0x100000041,
  gog_generator_sound = 0x100000041,
  windmill_sound = 0x100000042,
  whirlpool_sound = 0x100000043,
  wolf_rider_generator_sound = 0x100000044,
  shore_sound = 0x100000045,
  max_environment_sounds = 0x100000046,
};

enum ArtifactPrices
{
  const_free_artifact = 0x0,
  const_artifact_costs_2000 = 0x1,
  const_artifact_requires_wisdom = 0x2,
  const_artifact_requires_leadership = 0x3,
  const_artifact_costs_2500 = 0x4,
  const_artifact_costs_3000 = 0x5,
  const_artifact_defended = 0x6,
};

enum TCarryOverPoolNumber
{
  e_pool_1 = 0x0,
  e_pool_2 = 0x1,
  e_pool_choice = 0x2,
  e_pool_both = 0x3,
};

enum type_action_type
{
  const_initialization_action = 0x0,
  const_normal_action = 0x1,
  const_remote_action = 0x2,
  const_recorded_action = 0x3,
};

enum WiseTreePrices
{
  const_tree_wants_nothing = 0x0,
  const_tree_wants_gold = 0x1,
  const_tree_wants_gems = 0x2,
  const_tree_price_count = 0x3,
};

enum advManager::EBottomViewType
{
  BVTYPE_NONE = 0x0,
  BVTYPE_NEW_TURN = 0x1,
  BVTYPE_KINGDOM = 0x2,
  BVTYPE_HERO = 0x3,
  BVTYPE_TOWN = 0x4,
  BVTYPE_ENEMY_TURN = 0x5,
  BVTYPE_RESOURCE_MESSAGE = 0x6,
  BVTYPE_MESSAGE = 0x7,
  BVTYPE_HOLD = 0x8,
};

enum ScholarAwards
{
  const_scholar_primary_skill = 0x0,
  const_scholar_secondary_skill = 0x1,
  const_scholar_spell = 0x2,
};

enum TPrimarySkill
{
  ePriSkillAttack = 0x0,
  ePriSkillDefense = 0x1,
  ePriSkillPower = 0x2,
  ePriSkillKnowledge = 0x3,
  kNumPrimarySkills = 0x4,
  kMaxPrimarySkillLevel = 0x63,
};

enum SeaChestRewardTypes
{
  const_sea_chest_nothing = 0x0,
  const_sea_chest_gold = 0x1,
  const_sea_chest_artifact = 0x2,
};

enum TRoadType
{
  eRoadNone = 0x0,
  eRoadDirt = 0x1,
  eRoadGravel = 0x2,
  eRoadCobblestone = 0x3,
  kNumRoadTypes = 0x4,
};

enum TRiverType
{
  eRiverNone = 0x0,
  eRiverClear = 0x1,
  eRiverIcy = 0x2,
  eRiverMuddy = 0x3,
  eRiverLava = 0x4,
  kNumRiverTypes = 0x5,
};

enum hero_seqid
{
  hs_stand_n = 0x0,
  hs_stand_ne = 0x1,
  hs_stand_e = 0x2,
  hs_stand_se = 0x3,
  hs_stand_s = 0x4,
  hs_walk_n = 0x5,
  hs_walk_ne = 0x6,
  hs_walk_e = 0x7,
  hs_walk_se = 0x8,
  hs_walk_s = 0x9,
  hs_turn_n_ne = 0xA,
  hs_turn_ne_n = 0xB,
  hs_turn_ne_e = 0xC,
  hs_turn_e_ne = 0xD,
  hs_turn_e_se = 0xE,
  hs_turn_se_e = 0xF,
  hs_turn_se_s = 0x10,
  hs_turn_s_se = 0x11,
  hs_max = 0x12,
};

enum TQuickCreatureWindow::TViewLevel
{
  ViewNone = 0x0,
  ViewAll = 0x1,
};

enum TQuickCreatureWindow::TDisposition
{
  Flee = 0x0,
  Attack = 0x1,
  Join = 0x2,
  JoinPrice = 0x3,
};

enum type_combat_cursor
{
  SELECT_NULL = 0x0,
  SELECT_MOVE = 0x1,
  SELECT_FLY = 0x2,
  SELECT_SPECIAL_ATTACK = 0x3,
  SELECT_TENT = 0x4,
  SELECT_ARMY_INFO = 0x5,
  CBT_ARROW_POINTER = 0x6,
  const_attack_northeast = 0x7,
  SELECT_ATTACK = 0x7,
  const_attack_east = 0x8,
  const_attack_southeast = 0x9,
  const_attack_southwest = 0xA,
  const_attack_west = 0xB,
  const_attack_northwest = 0xC,
  const_attack_up = 0xD,
  const_attack_down = 0xE,
  SELECT_SPECIAL_ATTACK_BROKEN = 0xF,
  SELECT_ATTACK_WALL = 0x10,
  SELECT_FIRST_AID = 0x11,
  SELECT_SACRIFICE = 0x12,
  SELECT_TELEPORT = 0x13,
  SELECT_CREATURE_SPELL = 0x14,
  SELECT_ENEMY_TENT = 0x15,
  const_select_tower_info = 0x16,
  COMBAT_CURSOR_MOVE = 0x100000001,
  COMBAT_CURSOR_FLY = 0x100000002,
  COMBAT_CURSOR_SPECIAL_ATTACK = 0x100000003,
  COMBAT_CURSOR_TENT = 0x100000004,
  COMBAT_CURSOR_ARMY_INFO = 0x100000005,
  COMBAT_CURSOR_ARROW_POINTER = 0x100000006,
  COMBAT_CURSOR_ATTACK_NORTHEAST = 0x100000007,
  COMBAT_CURSOR_ATTACK = 0x100000007,
  COMBAT_CURSOR_ATTACK_EAST = 0x100000008,
  COMBAT_CURSOR_ATTACK_SOUTHEAST = 0x100000009,
  COMBAT_CURSOR_ATTACK_SOUTWEST = 0x10000000A,
  COMBAT_CURSOR_ATTACK_WEST = 0x10000000B,
  COMBAT_CURSOR_ATTACK_NORTHWEST = 0x10000000C,
  COMBAT_CURSOR_ATTACK_UP = 0x10000000D,
  COMBAT_CURSOR_ATTACK_DOWN = 0x10000000E,
  COMBAT_CURSOR_SPECIAL_ATTACK_BROKEN = 0x10000000F,
  COMBAT_CURSOR_ATTACK_WALL = 0x100000010,
  COMBAT_CURSOR_FIRST_AID = 0x100000011,
  COMBAT_CURSOR_SACRIFICE = 0x100000012,
  COMBAT_CURSOR_TELEPORT = 0x100000013,
  COMBAT_CURSOR_CREATURE_SPELL = 0x100000014,
  COMBAT_CURSOR_SELECT_HERO_INFO = 0x100000015,
  COMBAT_CURSOR_SELECT_TOWER_INFO = 0x100000016,
};

enum TFortificationLevel
{
  eFortificationNone = 0x0,
  eFortificationFort = 0x1,
  eFortificationCitadel = 0x2,
  eFortificationCastle = 0x3,
};

enum combatManager::TWallSection
{
  eWallSectionDoor = 0x0,
  eWallSectionDoorRope = 0x1,
  eWallSectionMoat = 0x2,
  eWallSectionMoatLip = 0x3,
  eWallSectionBackWall = 0x4,
  eWallSectionUpperTower = 0x5,
  eWallSectionUpperWall = 0x6,
  eWallSectionUpperButtress = 0x7,
  eWallSectionMidUpperWall = 0x8,
  eWallSectionGate = 0x9,
  eWallSectionMidLowerWall = 0xA,
  eWallSectionLowerButtress = 0xB,
  eWallSectionLowerWall = 0xC,
  eWallSectionLowerTower = 0xD,
  eWallSectionMainBuilding = 0xE,
  eWallSectionMainBuildingCover = 0xF,
  eWallSectionLowerTowerCover = 0x10,
  eWallSectionUpperTowerCover = 0x11,
  kNumWallSections = 0x12,
};

enum combatManager::TDoorStatus
{
  DOOR_BROKEN = 0x0,
  DOOR_DOWN = 0x1,
  DOOR_DOWN_1 = 0x2,
  DOOR_UP = 0x3,
};

enum combatManager::TArcherID
{
  eArcherMainBuilding = 0x0,
  eArcherLowerTower = 0x1,
  eArcherUpperTower = 0x2,
  kNumArchers = 0x3,
};

enum combatManager::__unnamed
{
  INVALID_HEX_INDEX = 0xFFFFFFFF,
  LEFT_AMMOCART_GRIDINDEX = 0x100000012,
  RIGHT_AMMOCART_GRIDINDEX = 0x100000020,
  LEFT_BALLISTA_GRIDINDEX = 0x100000033,
  RIGHT_BALLISTA_GRIDINDEX = 0x100000043,
  DOOR_GRIDINDEX = 0x100000060,
  LEFT_FIRSTAID_GRIDINDEX = 0x100000099,
  RIGHT_FIRSTAID_GRIDINDEX = 0x1000000A9,
  LEFT_CATAPULT_GRIDINDEX = 0x200000077,
  RIGHT_CATAPULT_GRIDINDEX = 0x200000087,
  LOWER_WALL_RIGHT_GRIDINDEX = 0x2000000C8,
  LOWER_TOWER_DRAW_GRIDINDEX = 0x2000000FB,
  LEFT_GENERAL_GRIDINDEX = 0x2000000FC,
  RIGHT_GENERAL_GRIDINDEX = 0x2000000FD,
  MAIN_BUILDING_GRIDINDEX = 0x2000000FE,
  UPPER_TOWER_GRIDINDEX = 0x2000000FF,
  DRAWBRIDGE_HEX_Y = 0x300000005,
  DP_MIN = 0x400000000,
  DP_WALLS = 0x400000000,
  DP_DEAD_ARMIES = 0x400000001,
  DP_OBSTACLES = 0x400000002,
  DP_WALKING_UP = 0x400000003,
  DP_NORMAL = 0x400000004,
  DP_WINCING = 0x400000005,
  DP_ATTACKING = 0x400000006,
  DP_WALKING_DOWN = 0x400000007,
  DP_MAX = 0x400000007,
  DP_ALL = 0x400000008,
  NUM_SIDES = 0x500000002,
  MAX_COMBAT_ARMIES = 0x500000014,
  BASE_COMBAT_ANIMATION_PERIOD = 0x500000032,
  COMBAT_HERO_FIDGET_TIME = 0x500001194,
  MAX_WALL_LEVELS = 0x600000005,
};

enum hexcell::TAttributes
{
  IS_OBSTACLE_ORIGIN = 0x1,
  IS_BLOCKED = 0x2,
  IS_QUICKSAND = 0x4,
  IS_LANDMINE = 0x8,
  IS_FIREWALL = 0x10,
  IS_STONEWALL = 0x20,
  IS_OBSTACLIZED = 0x3F,
  IS_MAGIC_OBSTACLE = 0x10000003C,
};

enum CHeroSessions::eSessionStatus
{
  closed = 0x0,
  open = 0x1,
  password = 0x2,
};

struct TMainMenu : heroWindow
{
  bool bShowCDMessage;
  widget* RolloverWidget;
};

struct bitmapBorder16 : border
{
  Bitmap16Bit* borderBitmap16;
};

enum TMainMenu::EGameCommandIDs
{
  NEW_GAME_ID = 0x65,
  LOAD_GAME_ID = 0x66,
  HIGH_SCORE_ID = 0x67,
  CREDITS_ID = 0x68,
  SAVE_GAME_ID = 0x6A,
  RESTART_ID = 0x6B,
  MAIN_MENU_ID = 0x6C,
};

enum TMainMenu::EOtherWidgetIDs
{
};

enum TGameTypeWindow::EWidgetIDs
{
  SINGLE_ID = 0x64,
  CAMPAIGN_ID = 0x65,
  MULTIPLAYER_ID = 0x66,
  TUTORIAL_ID = 0x67,
  QUIT_ID = 0x68,
  NEW_LOAD_ID = 0x69,
};

enum TGameTypeWindow::EOtherWidgetIDs
{
  VERSION_ID = 0xCA,
};

enum TMultiPlayerWindow::EOtherWidgetIDs
{
  ONLINE_ID = 0x65,
  HOT_SEAT_ID = 0x66,
  IPX_ID = 0x67,
  TCP_ID = 0x68,
  MODEM_ID = 0x69,
  DIRECT_ID = 0x6A,
  HOST_ID = 0x6B,
  JOIN_ID = 0x6C,
  SEARCH_ID = 0x6D,
  GAMENAME_1_ID = 0x6E,
  GAMENAME_2_ID = 0x6F,
  GAMENAME_3_ID = 0x70,
  GAMENAME_4_ID = 0x71,
  GAMENAME_5_ID = 0x72,
  GAMENAME_6_ID = 0x73,
  GAMENAME_7_ID = 0x74,
  GAMENAME_8_ID = 0x75,
  GAMENAME_9_ID = 0x76,
  GAMENAME_10_ID = 0x77,
  GAMENAME_11_ID = 0x78,
  GAMENAME_12_ID = 0x79,
  PLAYER_NAME_ID = 0x7D,
  IP_ADDRESS = 0x7E,
  SESS_NAME_HEADER_ID = 0x7F,
  USER_NAME_HEADER_ID = 0x80,
  HERO_SPLASH_ID = 0x81,
};

enum TSingleSelectionWindow::EWidgetIDs
{
};

enum TSingleSelectionWindow::EOtherWidgetIDs
{
  HELP_START = 0x64,
  SCENARIO_BACKGROUND_ID = 0x65,
  ADVANCED_BACKGROUND_ID = 0x66,
  SCENARIO_NAME_ID = 0x67,
  SCENARIO_DESC_ID = 0x68,
  EASY_ID = 0x69,
  NORMAL_ID = 0x6A,
  HARD_ID = 0x6B,
  EXPERT_ID = 0x6C,
  IMPOSSIBLE_ID = 0x6D,
  ALLY_FLAG1_ID = 0x6E,
  ALLY_FLAG2_ID = 0x6F,
  ALLY_FLAG3_ID = 0x70,
  ALLY_FLAG4_ID = 0x71,
  ALLY_FLAG5_ID = 0x72,
  ALLY_FLAG6_ID = 0x73,
  ALLY_FLAG7_ID = 0x74,
  ALLY_FLAG8_ID = 0x75,
  ENEMY_FLAG1_ID = 0x76,
  ENEMY_FLAG2_ID = 0x77,
  ENEMY_FLAG3_ID = 0x78,
  ENEMY_FLAG4_ID = 0x79,
  ENEMY_FLAG5_ID = 0x7A,
  ENEMY_FLAG6_ID = 0x7B,
  ENEMY_FLAG7_ID = 0x7C,
  ENEMY_FLAG8_ID = 0x7D,
  SCENARIO_OPTIONS_ID = 0x7E,
  ADVANCED_OPTIONS_ID = 0x7F,
  CHAT_DESC_ID = 0x80,
  MAP_DIFFICULTY_ID = 0x81,
  RATING_ID = 0x82,
  MAP_SIZE_ID = 0x83,
  VICTORY_CONDITION_ID = 0x84,
  LOSS_CONDITION_ID = 0x85,
  SMALL_FILTER_ID = 0x86,
  MEDIUM_FILTER_ID = 0x87,
  LARGE_FILTER_ID = 0x88,
  EXTRALARGE_FILTER_ID = 0x89,
  ALL_FILTER_ID = 0x8A,
  SCENARIO_NAME_1_ID = 0x8B,
  SCENARIO_NAME_2_ID = 0x8C,
  SCENARIO_NAME_3_ID = 0x8D,
  SCENARIO_NAME_4_ID = 0x8E,
  SCENARIO_NAME_5_ID = 0x8F,
  SCENARIO_NAME_6_ID = 0x90,
  SCENARIO_NAME_7_ID = 0x91,
  SCENARIO_NAME_8_ID = 0x92,
  SCENARIO_NAME_9_ID = 0x93,
  SCENARIO_NAME_10_ID = 0x94,
  SCENARIO_NAME_11_ID = 0x95,
  SCENARIO_NAME_12_ID = 0x96,
  SCENARIO_NAME_13_ID = 0x97,
  SCENARIO_NAME_14_ID = 0x98,
  SCENARIO_NAME_15_ID = 0x99,
  SCENARIO_NAME_16_ID = 0x9A,
  SCENARIO_NAME_17_ID = 0x9B,
  SCENARIO_NAME_18_ID = 0x9C,
  ENTRY_SCENARIO_NAME_1_ID = 0x9D,
  ENTRY_SCENARIO_NAME_2_ID = 0x9E,
  ENTRY_SCENARIO_NAME_3_ID = 0x9F,
  ENTRY_SCENARIO_NAME_4_ID = 0xA0,
  ENTRY_SCENARIO_NAME_5_ID = 0xA1,
  ENTRY_SCENARIO_NAME_6_ID = 0xA2,
  ENTRY_SCENARIO_NAME_7_ID = 0xA3,
  ENTRY_SCENARIO_NAME_8_ID = 0xA4,
  ENTRY_SCENARIO_NAME_9_ID = 0xA5,
  ENTRY_SCENARIO_NAME_10_ID = 0xA6,
  ENTRY_SCENARIO_NAME_11_ID = 0xA7,
  ENTRY_SCENARIO_NAME_12_ID = 0xA8,
  ENTRY_SCENARIO_NAME_13_ID = 0xA9,
  ENTRY_SCENARIO_NAME_14_ID = 0xAA,
  ENTRY_SCENARIO_NAME_15_ID = 0xAB,
  ENTRY_SCENARIO_NAME_16_ID = 0xAC,
  ENTRY_SCENARIO_NAME_17_ID = 0xAD,
  ENTRY_SCENARIO_NAME_18_ID = 0xAE,
  TURN_DURATION_ID = 0xAF,
  CHAT_ENTRY_ID = 0xB1,
  CHAT_PLUG_ID = 0xB2,
  CHAT_PLAYER_ID = 0xB3,
  CHAT_PLAYER_SLIDER = 0xB4,
  NEXT_ID = 0xB6,
  BEGIN_ID = 0xB7,
  BACK_ID = 0xB8,
  SSEXIT_ID = 0xB9,
  WHICHMAP_ID = 0xBA,
  SORT_NUMBER_ID = 0xBB,
  SORT_SIZE_ID = 0xBC,
  SORT_ALPHA_ID = 0xBD,
  SORT_VICTORY_ID = 0xBE,
  SORT_LOSS_ID = 0xBF,
  BRIEFING_ID = 0xC0,
  SSHERO_ID = 0xC1,
  GOLDPIC_ID = 0xC2,
  HUMAN_CPU1_ID = 0xC3,
  HUMAN_CPU2_ID = 0xC4,
  HUMAN_CPU3_ID = 0xC5,
  HUMAN_CPU4_ID = 0xC6,
  HUMAN_CPU5_ID = 0xC7,
  HUMAN_CPU6_ID = 0xC8,
  HUMAN_CPU7_ID = 0xC9,
  HUMAN_CPU8_ID = 0xCA,
  HANDICAP1_ID = 0xCB,
  HANDICAP2_ID = 0xCC,
  HANDICAP3_ID = 0xCD,
  HANDICAP4_ID = 0xCE,
  HANDICAP5_ID = 0xCF,
  HANDICAP6_ID = 0xD0,
  HANDICAP7_ID = 0xD1,
  HANDICAP8_ID = 0xD2,
  TOWN_LEFT1_ID = 0xD3,
  TOWN_LEFT2_ID = 0xD4,
  TOWN_LEFT3_ID = 0xD5,
  TOWN_LEFT4_ID = 0xD6,
  TOWN_LEFT5_ID = 0xD7,
  TOWN_LEFT6_ID = 0xD8,
  TOWN_LEFT7_ID = 0xD9,
  TOWN_LEFT8_ID = 0xDA,
  TOWN_RITE1_ID = 0xDB,
  TOWN_RITE2_ID = 0xDC,
  TOWN_RITE3_ID = 0xDD,
  TOWN_RITE4_ID = 0xDE,
  TOWN_RITE5_ID = 0xDF,
  TOWN_RITE6_ID = 0xE0,
  TOWN_RITE7_ID = 0xE1,
  TOWN_RITE8_ID = 0xE2,
  HERO_LEFT1_ID = 0xE3,
  HERO_LEFT2_ID = 0xE4,
  HERO_LEFT3_ID = 0xE5,
  HERO_LEFT4_ID = 0xE6,
  HERO_LEFT5_ID = 0xE7,
  HERO_LEFT6_ID = 0xE8,
  HERO_LEFT7_ID = 0xE9,
  HERO_LEFT8_ID = 0xEA,
  HERO_RITE1_ID = 0xEB,
  HERO_RITE2_ID = 0xEC,
  HERO_RITE3_ID = 0xED,
  HERO_RITE4_ID = 0xEE,
  HERO_RITE5_ID = 0xEF,
  HERO_RITE6_ID = 0xF0,
  HERO_RITE7_ID = 0xF1,
  HERO_RITE8_ID = 0xF2,
  RES_LEFT1_ID = 0xF3,
  RES_LEFT2_ID = 0xF4,
  RES_LEFT3_ID = 0xF5,
  RES_LEFT4_ID = 0xF6,
  RES_LEFT5_ID = 0xF7,
  RES_LEFT6_ID = 0xF8,
  RES_LEFT7_ID = 0xF9,
  RES_LEFT8_ID = 0xFA,
  RES_RITE1_ID = 0xFB,
  RES_RITE2_ID = 0xFC,
  RES_RITE3_ID = 0xFD,
  RES_RITE4_ID = 0xFE,
  RES_RITE5_ID = 0xFF,
  RES_RITE6_ID = 0x100,
  RES_RITE7_ID = 0x101,
  RES_RITE8_ID = 0x102,
  HERO_POSITION_1_ID = 0x103,
  HERO_POSITION_2_ID = 0x104,
  HERO_POSITION_3_ID = 0x105,
  HERO_POSITION_4_ID = 0x106,
  HERO_POSITION_5_ID = 0x107,
  HERO_POSITION_6_ID = 0x108,
  HERO_POSITION_7_ID = 0x109,
  HERO_POSITION_8_ID = 0x10A,
  TOWN_SETTING_1_ID = 0x10B,
  TOWN_SETTING_2_ID = 0x10C,
  TOWN_SETTING_3_ID = 0x10D,
  TOWN_SETTING_4_ID = 0x10E,
  TOWN_SETTING_5_ID = 0x10F,
  TOWN_SETTING_6_ID = 0x110,
  TOWN_SETTING_7_ID = 0x111,
  TOWN_SETTING_8_ID = 0x112,
  SLIDER_CHATWINDOW_ID = 0x113,
  SLIDER_FILEMENU_ID = 0x114,
  SLIDER_DURATION_ID = 0x115,
  PLAYER_HANDICAP_HEADER_ID = 0x116,
  TURN_DURATION_HEADER_ID = 0x117,
  STARTING_TOWN_HEADER_ID = 0x118,
  STARTING_HERO_HEADER_ID = 0x119,
  STARTING_BONUS_HEADER_ID = 0x11A,
  HELP_END = 0x11A,
  HERO_SETTING_1_ID = 0x11B,
  HERO_SETTING_2_ID = 0x11C,
  HERO_SETTING_3_ID = 0x11D,
  HERO_SETTING_4_ID = 0x11E,
  HERO_SETTING_5_ID = 0x11F,
  HERO_SETTING_6_ID = 0x120,
  HERO_SETTING_7_ID = 0x121,
  HERO_SETTING_8_ID = 0x122,
  PLAYER_NAME_EDIT_1 = 0x123,
  PLAYER_NAME_EDIT_2 = 0x124,
  PLAYER_NAME_EDIT_3 = 0x125,
  PLAYER_NAME_EDIT_4 = 0x126,
  PLAYER_NAME_EDIT_5 = 0x127,
  PLAYER_NAME_EDIT_6 = 0x128,
  PLAYER_NAME_EDIT_7 = 0x129,
  PLAYER_NAME_EDIT_8 = 0x12A,
  NEW_LOAD_SAVE_TEXT_ID = 0x12B,
  FACE_ID_1 = 0x12C,
  FACE_ID_2 = 0x12D,
  FACE_ID_3 = 0x12E,
  FACE_ID_4 = 0x12F,
  FACE_ID_5 = 0x130,
  FACE_ID_6 = 0x131,
  FACE_ID_7 = 0x132,
  FACE_ID_8 = 0x133,
  TOWN_ID_1 = 0x134,
  TOWN_ID_2 = 0x135,
  TOWN_ID_3 = 0x136,
  TOWN_ID_4 = 0x137,
  TOWN_ID_5 = 0x138,
  TOWN_ID_6 = 0x139,
  TOWN_ID_7 = 0x13A,
  TOWN_ID_8 = 0x13B,
  BONUS_ID_1 = 0x13C,
  BONUS_ID_2 = 0x13D,
  BONUS_ID_3 = 0x13E,
  BONUS_ID_4 = 0x13F,
  BONUS_ID_5 = 0x140,
  BONUS_ID_6 = 0x141,
  BONUS_ID_7 = 0x142,
  BONUS_ID_8 = 0x143,
  ENEMY_TEXT_ID = 0x144,
  FLAG_RCLICK_ID = 0x145,
  SAVE_GAME_STRIP = 0x146,
  VMS_ID_7 = 0x149,
  VMS_ID_6 = 0x14A,
  VMS_ID_5 = 0x14B,
  VMS_ID_4 = 0x14C,
  VMS_ID_3 = 0x14D,
  VMS_ID_2 = 0x14E,
  VMS_ID_1 = 0x14F,
  VMS_ID_0 = 0x150,
  SCN_OK_ID = 0x151,
  ADV_OK_ID = 0x152,
  DELETE_ID = 0x153,
  FREE_BLOCKS_ID = 0x154,
  BLOCKS_NEEDED_ID = 0x155,
  TIME_DATE_ID = 0x156,
  LAST_ID = 0x157,
};

enum creature_seqid
{
  cs_walk = 0x0,
  cs_fidget = 0x1,
  cs_wait = 0x2,
  cs_wince = 0x3,
  cs_defend = 0x4,
  cs_death = 0x5,
  cs_specdeath = 0x6,
  cs_turn_rf = 0x7,
  cs_turn_fr = 0x8,
  cs_turn_lf = 0x9,
  cs_turn_fl = 0xA,
  cs_attack_ur = 0xB,
  cs_attack_r = 0xC,
  cs_attack_dr = 0xD,
  cs_range_ur = 0xE,
  cs_range_r = 0xF,
  cs_range_dr = 0x10,
  cs_special_ur = 0x11,
  cs_special_r = 0x12,
  cs_special_dr = 0x13,
  cs_prewalk = 0x14,
  cs_postwalk = 0x15,
  cs_max = 0x16,
};

enum TLevelUpWindow::EOtherWidgetIDs
{
  TEXT1_ID = 0x7D2,
  TEXT2_ID = 0x7D3,
  TEXT3_ID = 0x7D4,
  TEXT4_ID = 0x7D5,
  TEXT5_ID = 0x7D6,
  TEXT6_ID = 0x7D7,
  TEXT7_ID = 0x7D8,
  PRISKILL_ID = 0x7D9,
  SKILLICON_1_ID = 0x7DA,
  SKILLICON_2_ID = 0x7DB,
  SKILLBORDER_1_ID = 0x7DC,
  SKILLBORDER_2_ID = 0x7DD,
};

enum THillFortWindow::EWidgetIDs
{
  HERO_PORTRAIT_ID = 0xCB,
  UPGRADE_ALL_BUTTON_ID = 0xCC,
  CREATURE_PORTRAIT_1_ID = 0xCD,
  CREATURE_PORTRAIT_2_ID = 0xCE,
  CREATURE_PORTRAIT_3_ID = 0xCF,
  CREATURE_PORTRAIT_4_ID = 0xD0,
  CREATURE_PORTRAIT_5_ID = 0xD1,
  CREATURE_PORTRAIT_6_ID = 0xD2,
  CREATURE_PORTRAIT_7_ID = 0xD3,
  CREATURE_NUM_1_ID = 0xD4,
  CREATURE_NUM_2_ID = 0xD5,
  CREATURE_NUM_3_ID = 0xD6,
  CREATURE_NUM_4_ID = 0xD7,
  CREATURE_NUM_5_ID = 0xD8,
  CREATURE_NUM_6_ID = 0xD9,
  CREATURE_NUM_7_ID = 0xDA,
  GOLD_ICON_1_ID = 0xDB,
  GOLD_ICON_2_ID = 0xDC,
  GOLD_ICON_3_ID = 0xDD,
  GOLD_ICON_4_ID = 0xDE,
  GOLD_ICON_5_ID = 0xDF,
  GOLD_ICON_6_ID = 0xE0,
  GOLD_ICON_7_ID = 0xE1,
  GOLD_COST_1_ID = 0xE2,
  GOLD_COST_2_ID = 0xE3,
  GOLD_COST_3_ID = 0xE4,
  GOLD_COST_4_ID = 0xE5,
  GOLD_COST_5_ID = 0xE6,
  GOLD_COST_6_ID = 0xE7,
  GOLD_COST_7_ID = 0xE8,
  RES_ICON_1_ID = 0xE9,
  RES_ICON_2_ID = 0xEA,
  RES_ICON_3_ID = 0xEB,
  RES_ICON_4_ID = 0xEC,
  RES_ICON_5_ID = 0xED,
  RES_ICON_6_ID = 0xEE,
  RES_ICON_7_ID = 0xEF,
  RES_COST_1_ID = 0xF0,
  RES_COST_2_ID = 0xF1,
  RES_COST_3_ID = 0xF2,
  RES_COST_4_ID = 0xF3,
  RES_COST_5_ID = 0xF4,
  RES_COST_6_ID = 0xF5,
  RES_COST_7_ID = 0xF6,
  TOTAL_RES_ICON_1_ID = 0xF7,
  TOTAL_RES_ICON_2_ID = 0xF8,
  TOTAL_RES_ICON_3_ID = 0xF9,
  TOTAL_RES_ICON_4_ID = 0xFA,
  TOTAL_RES_ICON_5_ID = 0xFB,
  TOTAL_RES_ICON_6_ID = 0xFC,
  TOTAL_RES_ICON_7_ID = 0xFD,
  TOTAL_RES_COST_1_ID = 0xFE,
  TOTAL_RES_COST_2_ID = 0xFF,
  TOTAL_RES_COST_3_ID = 0x100,
  TOTAL_RES_COST_4_ID = 0x101,
  TOTAL_RES_COST_5_ID = 0x102,
  TOTAL_RES_COST_6_ID = 0x103,
  TOTAL_RES_COST_7_ID = 0x104,
  UPGRADE_BUTTON_1_ID = 0x105,
  UPGRADE_BUTTON_2_ID = 0x106,
  UPGRADE_BUTTON_3_ID = 0x107,
  UPGRADE_BUTTON_4_ID = 0x108,
  UPGRADE_BUTTON_5_ID = 0x109,
  UPGRADE_BUTTON_6_ID = 0x10A,
  UPGRADE_BUTTON_7_ID = 0x10B,
};

struct THillFortWindow : heroWindow { };

enum recruitUnit::EWidgetIDs
{
};

enum type_event_record_type
{
  const_record_none = 0x0,
  const_record_move_hero = 0x1,
  const_record_teleport = 0x2,
  const_record_claim_mine = 0x3,
  const_record_claim_town = 0x4,
  const_record_hide_boat = 0x5,
  const_record_show_boat = 0x6,
  const_record_erase = 0x7,
  const_record_hide_hero = 0x8,
  const_record_show_hero = 0x9,
  const_record_player_death = 0xA,
  const_record_shroud = 0xB,
};

enum type_creature_bank_type
{
  const_cyclops_bank = 0x0,
  const_dwarf_bank = 0x1,
  const_griffin_bank = 0x2,
  const_imp_bank = 0x3,
  const_medusa_bank = 0x4,
  const_naga_bank = 0x5,
  const_dragonfly_bank = 0x6,
  const_shipwreck_bank = 0x7,
  const_derelict_bank = 0x8,
  const_sepulcher_bank = 0x9,
  const_dragon_bank = 0xA,
  const_creature_bank_types = 0xB,
  const_first_creature_bank = 0x100000000,
};

enum TCombatResultsWindow::EWidgetIDs
{
};

enum TCombatResultsWindow::EOtherWidgetIDs
{
  ATTACKER_NAME_ID = 0xC9,
  ATTACKER_PORTRAIT_ID = 0xCA,
  ATTACKER_STATUS_ID = 0xCB,
  DEFENDER_NAME_ID = 0xCC,
  DEFENDER_PORTRAIT_ID = 0xCD,
  DEFENDER_STATUS_ID = 0xCE,
  RESULTS_ID = 0xCF,
  ATTACKER_LOSSES_0_ID = 0xD0,
  ATTACKER_LOSSES_1_ID = 0xD1,
  ATTACKER_LOSSES_2_ID = 0xD2,
  ATTACKER_LOSSES_3_ID = 0xD3,
  ATTACKER_LOSSES_4_ID = 0xD4,
  ATTACKER_LOSSES_5_ID = 0xD5,
  ATTACKER_LOSSES_6_ID = 0xD6,
  DEFENDER_LOSSES_0_ID = 0xD7,
  DEFENDER_LOSSES_1_ID = 0xD8,
  DEFENDER_LOSSES_2_ID = 0xD9,
  DEFENDER_LOSSES_3_ID = 0xDA,
  DEFENDER_LOSSES_4_ID = 0xDB,
  DEFENDER_LOSSES_5_ID = 0xDC,
  DEFENDER_LOSSES_6_ID = 0xDD,
};

enum TCombatOptionsWindow::EOtherWidgetIDs
{
  DEFAULT_ID = 0xC9,
  AUTO_CREATURES_ID = 0xE1,
  AUTO_SPELLS_ID = 0xE2,
  AUTO_CATAPULT_ID = 0xE3,
  AUTO_BALLISTA_ID = 0xE4,
  AUTO_FIRST_AID_TENT_ID = 0xE5,
  ANIMATION_SPEED_0_ID = 0xE6,
  ANIMATION_SPEED_1_ID = 0xE7,
  ANIMATION_SPEED_2_ID = 0xE8,
  CREATURE_INFO_STATS_ID = 0xE9,
  CREATURE_INFO_SPELLS_ID = 0xEA,
  VIEW_HEX_ID = 0xEB,
  MOVEMENT_SHADOW_ID = 0xEC,
  SHADOW_CURSOR_ID = 0xED,
};

enum type_speed_category
{
  const_ranged = 0x0,
  const_very_fast = 0x1,
  const_fast = 0x2,
  const_average = 0x3,
  const_slow = 0x4,
  const_max_catagories = 0x5,
};

enum TSkuttleBoatWindow::EWidgetIDs
{
};

enum TSkuttleBoatWindow::EOtherWidgetIDs
{
};

enum TDimensionDoorWindow::EWidgetIDs
{
};

enum TDimensionDoorWindow::EOtherWidgetIDs
{
};

enum TTownGateWindow::EWidgetIDs
{
  TITLE_TEXT_ID = 0x1,
  SELECT_TEXT_ID = 0x2,
  TOWN_5_ID = 0x9,
  TOWN_6_ID = 0xA,
  TOWN_7_ID = 0xB,
  TOWN_8_ID = 0xC,
  SLIDER_ID = 0xE,
  NUM_TOWN_ENTRIES = 0x100000009,
};

enum TPuzzleWindow::EWidgetIDs
{
};

enum TPuzzleWindow::EOtherWidgetIDs
{
};

enum TAdventureOptionsWindow::EWidgetIDs
{
  VIEW_WORLD_ID = 0x1,
  VIEW_PUZZLE_ID = 0x2,
  VIEW_SCENARIO_ID = 0x3,
  DIG_ID = 0x4,
  REPLAY_ID = 0x5,
};

enum TAdventureOptionsWindow::EOtherWidgetIDs
{
};

enum TAdvMenu::EWidgetIDs
{
};

struct town
{
  int8 id;
  int8 playerOwner;
  uchar builtThisTurn;
  uchar threatening_heroes;
  int8 townType;
  uchar mapX;
  uchar mapY;
  uchar mapZ;
  uchar boatX;
  uchar boatY;
  int garrisonHero;
  int occupyingHero;
  int8 mageLevel;
  int16[7][2] population;
  int8 bIsGrouped;
  uchar ManaVortexFull;
  uchar pond_amount;
  EGameResource pond_resource;
  TCreatureType summoningType;
  int16 summoningPopulation;
  SpellID[6][5] townSpells;
  int8[5] maxTownSpellAvailable;
  std::string cName;
  std::bitset_70_ SpellDisabledMask;
  armyGroup town_army;
  int[7][2] generator_bonus;
  unsigned int64 populationMask;
  unsigned int64 full_building_mask;
  unsigned int64 legal_buildings;
};

enum TSystemOptionsWindow::EWidgetIDs
{
};

enum TSystemOptionsWindow::EOtherWidgetIDs
{
  MUSIC_VOLUME_0_ID = 0xC9,
  MUSIC_VOLUME_1_ID = 0xCA,
  MUSIC_VOLUME_2_ID = 0xCB,
  MUSIC_VOLUME_3_ID = 0xCC,
  MUSIC_VOLUME_4_ID = 0xCD,
  MUSIC_VOLUME_5_ID = 0xCE,
  MUSIC_VOLUME_6_ID = 0xCF,
  MUSIC_VOLUME_7_ID = 0xD0,
  MUSIC_VOLUME_8_ID = 0xD1,
  MUSIC_VOLUME_9_ID = 0xD2,
  EFFECTS_VOLUME_0_ID = 0xD3,
  EFFECTS_VOLUME_1_ID = 0xD4,
  EFFECTS_VOLUME_2_ID = 0xD5,
  EFFECTS_VOLUME_3_ID = 0xD6,
  EFFECTS_VOLUME_4_ID = 0xD7,
  EFFECTS_VOLUME_5_ID = 0xD8,
  EFFECTS_VOLUME_6_ID = 0xD9,
  EFFECTS_VOLUME_7_ID = 0xDA,
  EFFECTS_VOLUME_8_ID = 0xDB,
  EFFECTS_VOLUME_9_ID = 0xDC,
  MUSIC_TYPE_NONE_ID = 0xDD,
  MUSIC_TYPE_CD_ID = 0xDE,
  MUSIC_TYPE_MIDI_ID = 0xDF,
  HERO_SPEED_WALK_ID = 0xE0,
  HERO_SPEED_CANTER_ID = 0xE1,
  HERO_SPEED_GALLOP_ID = 0xE2,
  HERO_SPEED_JUMP_ID = 0xE3,
  AI_SPEED_CANTER_ID = 0xE4,
  AI_SPEED_GALLOP_ID = 0xE5,
  AI_SPEED_JUMP_ID = 0xE6,
  AI_SPEED_NONE_ID = 0xE7,
  WINDOW_SCROLL_SLOW = 0xE8,
  WINDOW_SCROLL_MEDIUM = 0xE9,
  WINDOW_SCROLL_FAST = 0xEA,
  SHOW_PATH_ID = 0xEB,
  MOVE_REMINDER_ID = 0xEC,
  QUICK_COMBAT_ID = 0xED,
  VIDEO_SUBTITLES_ID = 0xEE,
  VIDEO_QUALITY_LOW = 0xEF,
  VIDEO_QUALITY_HIGH = 0xF0,
  TOWN_OUTLINES_ID = 0xF1,
  ANIMATE_SPELLBOOK_ID = 0xF2,
};

enum TAdventureMapWindow::EWidgetIDs
{
  MAP_ID = 0x0,
  RADAR_ID = 0x1,
  SELECTION_WINDOW_ID = 0x2,
  KINGDOM_OVERVIEW_ID = 0x3,
  ELEVATION_TOGGLE_ID = 0x4,
  QUEST_LOG_ID = 0x5,
  SLEEP_ID = 0x6,
  MOVE_ID = 0x7,
  CAST_SPELL_ID = 0x8,
  ADVENTURE_OPTIONS_ID = 0x9,
  SYSTEM_OPTIONS_ID = 0xA,
  NEXT_HERO_ID = 0xB,
  END_TURN_ID = 0xC,
  HERO_UP_ID = 0xD,
  HERO_DOWN_ID = 0xE,
  HERO_0_ID = 0xF,
  HERO_1_ID = 0x10,
  HERO_2_ID = 0x11,
  HERO_3_ID = 0x12,
  HERO_4_ID = 0x13,
  HERO_MOVEMENT_0_ID = 0x14,
  HERO_MOVEMENT_1_ID = 0x15,
  HERO_MOVEMENT_2_ID = 0x16,
  HERO_MOVEMENT_3_ID = 0x17,
  HERO_MOVEMENT_4_ID = 0x18,
  HERO_MANA_0_ID = 0x19,
  HERO_MANA_1_ID = 0x1A,
  HERO_MANA_2_ID = 0x1B,
  HERO_MANA_3_ID = 0x1C,
  HERO_MANA_4_ID = 0x1D,
  TOWN_UP_ID = 0x1E,
  TOWN_DOWN_ID = 0x1F,
  TOWN_3_ID = 0x23,
  TOWN_4_ID = 0x24,
  CHAT_TEXT_ID = 0x25,
  CHAT_EDIT_ID = 0x26,
  NUM_HERO_BUTTONS = 0x100000005,
};

enum TAdventureMapWindow::EOtherWidgetIDs
{
  ROLLOVER_ID = 0xC8,
};

enum button::TButtonStates
{
  eButtonNormal = 0x0,
  eButtonSelected = 0x1,
  eButtonDisabled = 0x2,
  eButtonHighlighted = 0x3,
};

enum TResourceDisplay::EWidgetIDs
{
  WOOD_ID = 0x3E9,
  MERCURY_ID = 0x3EA,
  ORE_ID = 0x3EB,
  SULFUR_ID = 0x3EC,
  CRYSTAL_ID = 0x3ED,
  GEMS_ID = 0x3EE,
  GOLD_ID = 0x3EF,
  DAY_ID = 0x3F0,
  WOOD_ICON_ID = 0x3F1,
  MERCURY_ICON_ID = 0x3F2,
  ORE_ICON_ID = 0x3F3,
  SULFUR_ICON_ID = 0x3F4,
  CRYSTAL_ICON_ID = 0x3F5,
  GEMS_ICON_ID = 0x3F6,
  GOLD_ICON_ID = 0x3F7,
};

enum TTownMenu::EWidgetIDs
{
  BACKGROUND_ID = 0x0,
};

enum type_garrison_base_window::EWidgetIDs
{
  ROLLOVER_BACK_ID = 0xC8,
  ROLLOVER_TEXT_ID = 0xC9,
  ICON_ID = 0xCA,
  TITLE_ID = 0xCB,
};

enum TCastleWindow::EWidgetIDs
{
  DELAY = 0x64,
  UP_ID = 0x65,
  DOWN_ID = 0x66,
};

enum TSeerRewardType
{
  eRewardNone = 0x0,
  eRewardExperience = 0x1,
  eRewardMana = 0x2,
  eRewardMorale = 0x3,
  eRewardLuck = 0x4,
  eRewardResource = 0x5,
  eRewardPrimarySkill = 0x6,
  eRewardSecondarySkill = 0x7,
  eRewardArtifact = 0x8,
  eRewardSpell = 0x9,
  eRewardCreature = 0xA,
};

enum TTownScreenWindow::EWidgetIDs
{
  CREST_ID = 0x64,
  TOWN_GARRISON_0_ID = 0x65,
  TOWN_GARRISON_1_ID = 0x66,
  TOWN_GARRISON_2_ID = 0x67,
  TOWN_GARRISON_3_ID = 0x68,
  TOWN_GARRISON_4_ID = 0x69,
  TOWN_GARRISON_5_ID = 0x6A,
  TOWN_GARRISON_6_ID = 0x6B,
  TOWN_GARRISON_0_TEXT_ID = 0x6C,
  TOWN_GARRISON_1_TEXT_ID = 0x6D,
  TOWN_GARRISON_2_TEXT_ID = 0x6E,
  TOWN_GARRISON_3_TEXT_ID = 0x6F,
  TOWN_GARRISON_4_TEXT_ID = 0x70,
  TOWN_GARRISON_5_TEXT_ID = 0x71,
  TOWN_GARRISON_6_TEXT_ID = 0x72,
  TOWN_GARRISON_0_SELECTOR_ID = 0x73,
  TOWN_GARRISON_1_SELECTOR_ID = 0x74,
  TOWN_GARRISON_2_SELECTOR_ID = 0x75,
  TOWN_GARRISON_3_SELECTOR_ID = 0x76,
  TOWN_GARRISON_4_SELECTOR_ID = 0x77,
  TOWN_GARRISON_5_SELECTOR_ID = 0x78,
  TOWN_GARRISON_6_SELECTOR_ID = 0x79,
  GARRISON_PORTRAIT_ID = 0x7A,
  GARRISON_PORTRAIT_SELECTOR_ID = 0x7B,
  PORTRAIT_ID = 0x7C,
  VISITING_PORTRAIT_SELECTOR_ID = 0x7D,
  HERO_ARMY_0_ID = 0x7E,
  HERO_ARMY_1_ID = 0x7F,
  HERO_ARMY_2_ID = 0x80,
  HERO_ARMY_3_ID = 0x81,
  HERO_ARMY_4_ID = 0x82,
  HERO_ARMY_5_ID = 0x83,
  HERO_ARMY_6_ID = 0x84,
  HERO_ARMY_0_TEXT_ID = 0x85,
  HERO_ARMY_1_TEXT_ID = 0x86,
  HERO_ARMY_2_TEXT_ID = 0x87,
  HERO_ARMY_3_TEXT_ID = 0x88,
  HERO_ARMY_4_TEXT_ID = 0x89,
  HERO_ARMY_5_TEXT_ID = 0x8A,
  HERO_ARMY_6_TEXT_ID = 0x8B,
  HERO_ARMY_0_SELECTOR_ID = 0x8C,
  HERO_ARMY_1_SELECTOR_ID = 0x8D,
  HERO_ARMY_2_SELECTOR_ID = 0x8E,
  HERO_ARMY_3_SELECTOR_ID = 0x8F,
  HERO_ARMY_4_SELECTOR_ID = 0x90,
  HERO_ARMY_5_SELECTOR_ID = 0x91,
  HERO_ARMY_6_SELECTOR_ID = 0x92,
  PANORAMA_ID = 0x93,
  TOWN_BOTTOM_ID = 0x94,
  TOWN_NAME_ID = 0x95,
  TOWN_PORTRAIT_ID = 0x96,
  TOWN_TEXT_ID = 0x97,
  TOWN_UP_ARROW_ID = 0x98,
  TOWN_DOWN_ARROW_ID = 0x99,
  DIVIDE_ID = 0x9A,
  TOWN_0_ID = 0x9B,
  TOWN_1_ID = 0x9C,
  TOWN_2_ID = 0x9D,
  HALL_ICON_ID = 0x9E,
  CASTLE_ICON_ID = 0x9F,
  INCOME_TEXT_ID = 0xA0,
  CREST_ICONS = 0xA1,
  HERO_ICONS = 0xA2,
  SELECTOR_ID = 0xA3,
  BONUS_0_ID = 0xA4,
  BONUS_1_ID = 0xA5,
  BONUS_2_ID = 0xA6,
  BONUS_3_ID = 0xA7,
  BONUS_4_ID = 0xA8,
  BONUS_5_ID = 0xA9,
  BONUS_6_ID = 0xAA,
  BONUS_7_ID = 0xAB,
  BONUS_0_TEXT_ID = 0xAC,
  BONUS_1_TEXT_ID = 0xAD,
  BONUS_2_TEXT_ID = 0xAE,
  BONUS_3_TEXT_ID = 0xAF,
  BONUS_4_TEXT_ID = 0xB0,
  BONUS_5_TEXT_ID = 0xB1,
  BONUS_6_TEXT_ID = 0xB2,
  BONUS_7_TEXT_ID = 0xB3,
  TOWN_POP_ID = 0xB4,
  HALL_DOWN = 0xB5,
  HALL_UP = 0xB6,
  GARRISON_ID = 0xB7,
  NUM_TOWN_BUTTONS = 0x100000002,
};

struct TSingleSelectionWindow : CAdvPopup
{
  unsigned int clickTime;
  bool loadGameMode;
  bool saveGameMode;
  bool showRandomMaps;
  byte[1] gap_67;
  int textIndex;
  CSprite* VersionIcon;
  CSprite* VictoryIcon;
  CSprite* LossIcon;
  CSprite* TownPix;
  CSprite* Resource;
  CSprite* heroSpecificAbility;
  Bitmap816* GoldBox;
  Bitmap816*[8] Flags;
  Bitmap816*[8] Panels;
  Bitmap816*[163] HeroPix;
  Bitmap816* randomTownQuestion;
  Bitmap816* randomHeroQuestion;
  Bitmap816* randomTown;
  Bitmap816* randomHero;
  Bitmap816* noDice;
  Bitmap816* noHero;
  bool sortDescending;
  byte[3] gap_36D;
  int currentIndex;
  int currentMap;
  int durationIndex;
  bool inAdvancedOptions;
  bool inScenarioOptions;
  bool inRandomMapOptions;
  bool randomMapGeneration;
  textEntryWidget* saveGameEdit;
  byte[4] gap_384;
  CNewPlayerUpdateMan* pNewPlayerUpdateMan;
  GameSelectionHeadersStruct pendingRandomMap;
  std::vector_GameSelectionHeadersStruct_ mapsList;
  std::vector_GameSelectionHeadersStruct_ randomMapsList;
  std::vector_GameSelectionHeadersStruct_ currentMapsList;
  GameSelectionHeadersStruct* selectedMap;
  CNetPlayerHandler netPlayerHandler;
  bool receivedMaps;
  byte[3] gap_1835;
  CChatSlider* chatSlider;
  slider* fileSlider;
  slider* durationSlider;
  byte[4] gap_1844;
  CChatWidget* chatWidget;
  textWidget* nameList1;
  textWidget* nameList2;
  bool mapChanged;
  bool readingMaps;
  int8[2] gap_1856;
  CChatEdit* chatEdit;
  int sortWhich;
  int filterSize;
  bool scenarioOptionsStarted;
  bool chatShowing;
  int8[2] gap_1866;
  textButton* chatToggle;
  bool receivingMaps;
  int8[3] gap_186D;
  CSaveScreen* flagBack;
  int8[20] cGameVersion;
  CSingleSelectionNetMsgHandler netMsgHandler;
  int gameVersion;
  int gap_189C;
  int randomMapSize;
  bool32 randomMapTwoLevels;
  int randomMapHumans;
  int randomMapTeams;
  int randomMapComputers;
  int randomMapComputerTeams;
  int randomMapWater;
  int randomMapMonsterStrength;
  button*[9] randomMapPlayerButtons;
  button*[9] randomMapComputerButtons;
  button*[9] randomMapTeamButtons;
  button*[8] randomMapComputerTeamButtons;
  button*[4] randomMapWaterButtons;
  button*[4] randomMapMonsterStrengthButtons;
  type_text_scroller* mapDescScroller;
};

struct CNetPlayerHandler
{
  CNetPlayerHandlerPlayer[8] humanPlayers;
  CNetPlayerHandlerPlayer[8] computerPlayers;
  int playerPos;
  int playersCount;
  int unused;
  int assignedPos;
};

struct CNewPlayerUpdateMan { };

struct CMFCToolBarInfo { };

struct MemorySampleStructure
{
  HANDLE memHSample;
  void* data;
  uint size;
  int memCindex;
  int memVolume;
  int memLooping;
};

struct sample : resource
{
  MemorySampleStructure memSample;
};

struct GameTime { };

struct type_ballistics_traits
{
  int8 chance_to_hit_main_building;
  int8 chance_to_hit_tower;
  int8 chance_to_hit_drawbridge;
  int8 chance_to_hit_wall;
  int8 shots;
  int8[3] chance_to_inflict_damage;
};

struct searchArray
{
  uint maxQueueCount;
  uchar pay_transition_costs;
  int this_turns_movement;
  int land_movement;
  int sea_movement;
  bool can_summon_boat;
  bool can_cast_teleport;
  bool can_cast_flight;
  bool can_cast_water_walk;
  TSkillMastery water_walk_level;
  TSkillMastery flight_level;
  bool limit_reached;
  pathCell* cellData;
  tagRECT valid_rectangle;
  std::vector_pathCell_ queue;
  std::vector_pathCell_ptr_ result;
  std::vector_pathCell_ptr_ visited_points;
  bool* bIsMoatSlowed;
  int* danger_zones;
};

struct pathCell : type_point
{
  unsigned int32 visited : 1;
  unsigned int32 bIsTrigger : 1;
  unsigned int32 in_boat : 1;
  unsigned int32 magic_forbidden : 1;
  unsigned int32 flying : 1;
  unsigned int32 water_walking : 1;
  unsigned int32 town_portal : 1;
  unsigned int32 dimension_door : 1;
  unsigned int32 castle_gate : 1;
  unsigned int32 can_stop : 1;
  unsigned int32 last_can_stop : 1;
  unsigned int32 direction : 4;
  unsigned int32 delta_x : 5;
  unsigned int32 delta_y : 5;
  unsigned int32 flight_cost : 6;
  type_point last_point;
  type_point monster;
  int barrier_value;
  int danger_value;
  ushort cost;
  ushort adjusted_cost;
  ushort move_left;
};

struct TArtifactTraits
{
  int8* m_name;
  int m_cost;
  int m_allowableSlotMask;
  int m_class;
  int8* m_description;
  CombinationArtifactType m_comboType;
  CombinationArtifactType m_targetCombo;
  bool m_disabled;
  bool m_givesSpells;
};

struct hexcell
{
  int16 refX;
  int16 refY;
  int16 hexULX;
  int16 hexULY;
  int16 hexBRX;
  int16 hexBRY;
  int16 fullHexBRY;
  uint attributes;
  int obstacleIndex;
  int8 armyGrp;
  int8 armyIndex;
  int8 partOfDouble;
  int iBodiesInHex;
  int8[14] deadArmyGrp;
  int8[14] deadArmyIndex;
  int8[14] deadPartOfDouble;
  bool bValidMove;
  uchar front_move;
  uchar mouse_shaded;
  int8 background_offset;
  SLimitData obstacleLimitData;
  SLimitData cloudLimitData;
};

struct SLimitData
{
  int MinX;
  int MinY;
  int MaxX;
  int MaxY;
};

struct CNetMsg
{
  int m_from;
  unsigned int m_dpidFrom;
  CNetMsg::eRS_Messages m_subType;
  unsigned int m_size;
  unsigned int m_UncompressedSize;
};

struct CNetMsgHandler
{
  CNetMsgHandler::vftable_t* vftable;
  uchar m_inPopup;
  CNetMsg* m_pAbortPopupMsg;
};

struct CAdvPopup : CHeroWindowEx
{
  int exitId;
  int exitCodeSubtype;
  int exitCommand;
  bool netHandlerInPopup;
};

struct TSubWindow
{
  TSubWindow::vftable_union_t vftable;
  int X;
  int Y;
  int Width;
  int Height;
  TWidgetVector Widgets;
  heroWindow* ParentWindow;
  int FirstWidgetID;
  int LastWidgetID;
  Bitmap16Bit* Background;
};

struct TBottomViewMessage : type_bottom_view_window { };

struct type_bottom_view_window : TSubWindow { };

struct TBottomViewNewTurn : type_bottom_view_window { };

struct TCombatHeroSubWindow : TSubWindow
{
  bitmapBorder* background;
  bitmapBorder* portrait;
  textWidget* attackText;
  textWidget* defenseText;
  textWidget* powerText;
  textWidget* knowledgeText;
  iconWidget* moraleIcon;
  iconWidget* luckIcon;
  textWidget* manaText;
  bool shown;
};

struct TCombatCreatureSubWindow : TSubWindow { };

struct soundNode
{
  e_looping_sound_id soundID;
  int priority;
};

struct NewfullMap
{
  std::vector_CObjectType_ ObjectTypes;
  std::vector_CObject_ Objects;
  std::vector_CSprite_ptr_ Sprites;
  std::vector_TreasureData_ CustomTreasureList;
  std::vector_MonsterData_ CustomMonsterList;
  std::vector_BlackBoxData_ BlackBoxList;
  std::vector_TSeerHut_ SeerHutList;
  std::vector_TQuestGuard_ QuestGuardList;
  std::vector_TTimedEvent_ TimedEventList;
  std::vector_TTownEvent_ TownEventList;
  std::vector_HeroPlaceholder_ PlaceHolderList;
  std::vector_type_quest_ptr_ QuestList;
  std::vector_TRandomDwelling_ RandomDwellingList;
  NewmapCell* cellData;
  int Size;
  bool HasTwoLevels;
  std::vector_CObjectType_[232] ObjectTypeTables;
};

struct executive
{
  baseManager* headManager;
  baseManager* tailManager;
  baseManager* currentManager;
  int dialogReturn;
};

struct inputManager : baseManager
{
  message[64] iBuffer;
  int iHead;
  int iTail;
  int bufferBusy;
  int mouseInstalled;
  int16[128] scanCodeTable;
  int keyboardInstalled;
  int keyboardFilter;
  int keyCodeType;
  int extendFlag;
  int currWidgetID;
  int possibleWidgetID;
};

struct philAI { };

struct type_AI_player
{
  int16 team;
  int magus_hut_value;
  int[7] reserved_funds;
  int[7] resource_supply;
  int[7] resource_demand;
  unknown float[7] resource_value;
};

struct NewmapCell::TObjectCell
{
  int16 ObjectIndex;
  int8 CellX : 4;
  int8 CellY : 4;
  int8 Height;
};

struct CNetPlayerHandlerPlayer : CNetPlayerInfo
{
  int heroIndex;
  int townIndex;
  int availableHeroesCount;
  int[16] availableHeroes;
  int startBonusIndex;
  int playerPos;
  int color;
  int handicap;
};

struct TCastleWindow : CAdvPopup
{
  int scroll_offset;
  TResourceDisplay* CastleBank;
  iconWidget*[7] SpriteWidget;
  type_building_id castleType;
};

struct AI
{
  int[7] turnExpectedResource;
  int[7] turnProductionResource;
  byte[4] pad_38;
  unknown float[7] resource_value;
  int average_resource_value;
  float turnValueOfAvgArtifact;
};

struct type_AI_puzzle_tile
{
  int object_type_bf;
  int object_cords_bf;
  int terr_river_road_bf;
  int diggable_has_grail_visible_bf;
};

struct std::bitset_70_
{
  int8[12] bitset_array;
};

struct VictoryConditionStruct
{
  int8 Type;
  int8 AllowNormalVictory;
  int8 AppliesToComputer;
  int ArtifactNum;
  int CreatureType;
  int NumCreatures;
  EGameResource ResourceType;
  int NumResources;
  int TownX;
  int TownY;
  int TownZ;
  int8 HallLevel;
  int8 CastleLevel;
  int HeroX;
  int HeroY;
  int HeroZ;
  THeroID HeroID;
  int MonsterX;
  int MonsterY;
  int MonsterZ;
  int time_to_survive;
  uchar GameWon;
  int8 playerWinner;
};

struct LossConditionStruct
{
  int8 Type;
  int TownX;
  int TownY;
  int TownZ;
  int HeroX;
  int HeroY;
  int HeroZ;
  THeroID HeroID;
  int16 NumDays;
  bool GameLost;
  int8 playerLoser;
};

struct SGameSetupOptions
{
  int8[8] color;
  int8[8] handicap;
  TTownType[8] alignment;
  int8[8] playerPos;
  int8 difficulty;
  int8[251] cFilename;
  int8[100] cPath;
  int8[8] canFlipFromToComputer;
  int8 curSelectedPlayer;
  int8 bThisFileInitialized;
  int8 initializationNumHumans;
  int8 turnDuration;
  THeroID[8] startingHero;
  int8[8] startingBonus;
};

struct std::map_int_HeroPlayerInfo_
{
  int8 allocator;
  int8 key_compare;
  std::map_int_HeroPlayerInfo_::_Node* _Head;
  bool _Multi;
  size_t _Size;
};

struct CMapHeaderData::TPlayerSlotAttributes
{
  bool CanBeHuman;
  bool CanBeComputer;
  int AIStrategy;
  int16 legal_alignments;
  bool HasRandomAlignment;
  bool GenerateHero;
  bool has_main_town;
  int main_town_type;
  type_point CastleLoc;
  int8 hasRandomHero;
  THeroID nonRandomHeroId;
  THeroID nonRandomHeroCustomPortrait;
  int8[12] nonRandomHeroCustomName;
  int default_placeholders;
  std::vector player_heroes;
};

struct CMapHeaderData
{
  int iVersion;
  bool IsPlayable;
  bool iDifficulty;
  bool numPlayers;
  bool minNumHumanPlayers;
  bool maxNumHumanPlayers;
  bool lastTownNameAssigned;
  bool mapHasNotBeenSaved;
  int8 max_hero_level;
  int8 numTeams;
  int8[8] teamInfo;
  int Size;
  bool HasTwoLayers;
  std::vector_int_ placeholders;
  VictoryConditionStruct victory_condition;
  LossConditionStruct loss_condition;
  CMapHeaderData::TPlayerSlotAttributes[8] PlayerSlotAttributes;
};

struct std::bitset_156_
{
  byte[20] bitset_array;
};

struct SCampaign
{
  bool bIsCheater;
  bool bSecretActive;
  int8 iCurMap;
  byte[1] align1;
  ECampaignType iCurrentCampaign;
  int NumMapRegions;
  int8 iCrossoverArrayIndex;
  int briefing_choice;
  std::string CampaignFilename;
  bool[21] bCampaignCompleted;
  byte[3] align2;
  std::vector_vector__hero__ carryover_pool;
  std::vector_vector__type_artifact__ carryover_artifact;
  std::vector_CampaignScenarioInfo_ scenarios;
  std::vector_int_ assigned_carryover;
};

struct NewSMapHeader : CMapHeaderData
{
  std::map_int_HeroPlayerInfo_ heroPlayerSetups;
  std::string mapName;
  std::string mapDescription;
  std::bitset_156_ availableHeroes;
};

struct type_AI_combat_parameters
{
  int lowest_attack;
  int lowest_defense;
  bool kills_only;
  uchar simulated;
  int friendly_combat_value;
  int enemy_combat_value;
  int awake_friendly_value;
  int awake_enemy_value;
  int rounds_left;
  int our_group;
  int enemy_group;
};

struct type_AI_enemy_data
{
  army* enemy;
  int damage;
  int count;
  int total_damage;
};

struct type_AI_spellcaster
{
  void* vftable;
  hero* current_hero;
  hero* enemy_hero;
  int our_group;
  int enemy_group;
  int enemy_can_attack;
  int can_be_attacked;
  uchar win_likely;
  uchar is_creature_spell;
  type_AI_combat_parameters estimate;
  type_AI_spellcaster* enemy_caster;
  uchar owns_enemy_caster;
  type_AI_enemy_data[20] melee_enemies;
  type_AI_enemy_data[20] ranged_enemies;
  type_AI_enemy_data[20] worst_enemies;
};

struct type_AI_combat_data
{
  std::vector_type_monster_data_ creatures;
  EMagicTerrain magic_terrain;
  int mana;
  bool can_cast_spells;
  int total_combat_value;
  int tactics_advantage;
  hero* current_hero;
  armyGroup* current_army;
  hero* enemy_hero;
  uchar wall_archery_penalty;
  int16 wall_speed_limit;
};

struct type_monster_data
{
  int index;
  TCreatureType type;
  int number;
  int original_number;
  int speed;
  unknown float melee_modifier;
  unknown float final_melee_modifier;
  unknown float ranged_modifier;
  unknown float combat_value_per_hit;
  type_speed_category category;
  int value;
  int total_value;
};

struct std::vector_type_monster_data_
{
  int8 allocator;
  type_monster_data* first;
  type_monster_data* last;
  type_monster_data* end;
};

struct type_enchant_data
{
  SpellID spell;
  TSkillMastery mastery;
  int power;
  int duration;
  bool check_resistance;
};

struct type_spell_choice : type_enchant_data
{
  int target_hex;
  int second_target_hex;
  int value;
  uchar cast_now;
};

struct highScoreManager : baseManager
{
  highScoreManager::HighScoreRec[11] scenarioRecords;
  highScoreManager::HighScoreRec[11] campaignRecords;
  int showSingleScenarios;
};

struct townManager : baseManager
{
  town* currTown;
  bitmapBorder16* panorama;
  CSprite*[7] MonPix;
  townObject*[44] objects;
  int numObjects;
  TTownType loadedTownType;
  void* unused;
  TTownScreenWindow* townWindow;
  strip* garrisonStrip;
  strip* heroStrip;
  strip* currStrip;
  int currIndex;
  strip* srcStrip;
  int srcIndex;
  strip* destStrip;
  int destIndex;
  TResourceDisplay* townBank;
  TResourceDisplay* townPopupBank;
  int8[80] townText;
  int lastHover;
  int lastQualifier;
  int command;
  int64 canBuyMask;
  int64 canBuildMask;
  CTownNetMsgHandler* pNetMsgHandler;
  CNetMsgHandler* pNetMsgHandlerSave;
  int objToBuild;
  CAdvPopup* dialogWindow;
  heroWindow* multiWin;
  int divideStatus;
  int recruitSelected;
  uchar[7] currentDwellingIDOff;
  int align;
};

struct std::set_SpellID_
{
  int8 allocator;
  int8 key_compare;
  std::set_SpellID_::_Node* _Head;
  bool _Multi;
  size_t _Size;
};

struct HeroExtra
{
  int8 Owner;
  byte[3] pad_2;
  THeroID id;
  int objRef;
  int8 bCustomName;
  int8[13] Name;
  bool customExperience;
  byte[1] pad_1B;
  int Experience;
  bool bCustomPortraitNumber;
  uchar PortraitNumber;
  bool bCustomSecondarySkills;
  byte[1] pad_23;
  int NumSecondarySkills;
  int8[8] secondarySkill;
  int8[8] secondarySkillLevel;
  bool bCustomArmies;
  byte[3] pad_39;
  TCreatureType[7] armies;
  int16[7] numTroops;
  int8 GroupFormation;
  bool bCustomArtifacts;
  type_artifact[19] artifacts;
  type_artifact[64] backpack;
  uchar numInBackpack;
  type_point location;
  int8 PatrolRadius;
  bool bCustomBiography;
  byte[1] pad_307;
  std::string sBiography;
  int sex;
  bool bCustomSpells;
  byte[3] pad_31D;
  std::bitset_70_ customSpells;
  bool bCustomPrimarySkills;
  int8[4] primarySkills;
  int8[3] aligned5;
};

struct type_town_threat_checker
{
  void* vftable;
  int current_player_id;
};

struct townObject
{
  int numFrames;
  int currFrame;
  int x;
  int y;
  int h;
  int w;
  int visible;
  int objId;
  CSprite* objIcon;
  Bitmap816* objOutline;
  Bitmap816* objHotspot;
  border* objBorder;
};

struct TTownScreenWindow : heroWindow
{
  int topTown;
  ushort* zBuffer;
  iconWidget*[8] growth_bonus_icon;
  textWidget*[8] growth_bonus_text;
  TCreatureType[8] bonus_creatures;
};

struct TTownMenu : CAdvPopup { };

struct CTownNetMsgHandler : CAdvMgrNetMsgHandler { };

struct strip { };

struct TResourceDisplay : TSubWindow
{
  bool IsSmall;
  textWidget*[7] ResourceWidgets;
  border*[7] ResourceIconWidgets;
  bitmapBorder* BackgroundWidget;
  textWidget* DayWidget;
};

struct TGiveResourceWindow : CAdvPopup { };

struct THallWindow : CAdvPopup { };

struct TSpreadsheetResource : resource
{
  std::vector_vector_char_ptr__ptr__ SpreadSheet;
  int8* Data;
  uint DataSize;
};

struct CDPlay
{
  CDPlay::vftable_union_t vftable;
  DPCAPS m_caps;
  IDirectPlay4* m_lpDP;
  GUID m_guid;
  int m_hRes;
  CAutoArray_CDPlaySession_* m_pSessionArray;
  CAutoArray_CDPlayConnection_* m_pConnectionArray;
  CAutoArray_CDPlayGroup_* m_pGroupArray;
  CAutoArray_CDPlayPlayer_* m_pPlayerArray;
  bool m_connected;
  bool m_inSession;
  bool m_isHost;
};

struct HeroDestination
{
  int value;
  int move_cost;
  uchar is_nearby;
  uchar is_critical;
};

struct type_horde_effect
{
  TCreatureType creature;
  int16 bonus;
  int16 dwelling;
};

enum creature_flags
{
  CF_DOUBLE_WIDE = 0x1,
  CF_FLYING_ARMY = 0x2,
  CF_SHOOTING_ARMY = 0x4,
  CF_HAS_EXTENDED_ATTACK = 0x8,
  CF_ALIVE = 0x10,
  CF_CATAPULT = 0x20,
  CF_SIEGE_WEAPON = 0x40,
  CF_KING_1 = 0x80,
  CF_KING_2 = 0x100,
  CF_KING_3 = 0x200,
  CF_IMMUNE_TO_MIND_SPELLS = 0x400,
  CF_SHOOTS_RAY = 0x800,
  CF_NO_MELEE_PENALTY = 0x1000,
  CF_UNUSED = 0x2000,
  CF_IMMUNE_TO_FIRE_SPELLS = 0x4000,
  CF_TWO_ATTACKS = 0x8000,
  CF_FREE_ATTACK = 0x10000,
  CF_NO_MORALE = 0x20000,
  CF_UNDEAD = 0x40000,
  CF_MULTI_HEADED = 0x80000,
  CF_FIREBALL_ATTACK = 0x100000,
  CF_IMMOBILIZED = 0x200000,
  CF_SUMMONED = 0x400000,
  CF_CLONE = 0x800000,
  CF_MORALE = 0x1000000,
  CF_WAITING = 0x2000000,
  CF_DONE = 0x4000000,
  CF_DEFENDING = 0x8000000,
  CF_SACRIFICED = 0x10000000,
  CF_RED_COLORING = 0x20000000,
  CF_GREY_COLORING = 0x40000000,
  CF_DRAGON = 0x80000000,
};

struct swapManager : baseManager
{
  heroWindow* parent;
  Bitmap816* border;
  hero*[2] heroes;
  int heroDonor;
  int heroReciever;
  int sourceSlot;
  int destSlot;
  bool32 slotClicked;
  byte align1;
  bool samePlayer;
  byte[2] align2;
  CNetMsgHandler* msgHandler;
  CSwapMgrNetMsgHandler* swapManagerMsgHandler;
};

struct TQuestLogWindow : CAdvPopup
{
  bool unknown;
  std::vector seerHutLogList;
};

struct TAdventureOptionsWindow : CAdvPopup { };

struct THeroSpecificAbility
{
  EHeroSpecificAbilityType type;
  THeroSpecificAbilityUnion info;
  int creatureAttackBonus;
  int creatureDefenseBonus;
  int creatureDamageBonus;
  TCreatureType creatureGrade;
  int unknown;
  int8* nameShort;
  int8* name;
  int8* description;
};

struct BinkManager
{
  DWORD Width;
  DWORD Height;
  DWORD TotalTime;
  DWORD FileFrameRate;
  DWORD FileFrameRateDiv;
  DWORD TotalOpenTime;
  DWORD TotalFrames;
  DWORD TotalPlayedFrames;
  DWORD SkippedFrames;
  DWORD SoundSkips;
  DWORD TotalBlitTime;
};

typedef CRITICAL_SECTION RTL_CRITICAL_SECTION;

typedef RTL_CRITICAL_SECTION _RTL_CRITICAL_SECTION;

struct LODFile
{
  FILE* fileptr;
  int8[256] LODFileName;
  int opened;
  uchar* dataBuffer;
  ulong dataBufferSize;
  int dataItemIndex;
  int dataPos;
  int matchindex;
  LODHeader header;
  int numEntries;
  std::vector subindex;
};

struct LODHeader
{
  int8[4] LOD_ID;
  int version;
  int numEntries;
  int8[80] reserved;
};

struct LODEntry
{
  int8[16] name;
  int offset;
  int size;
  int attrib;
  int csize;
};

struct CRect : tagRECT { };

struct CImmProject
{
  LPVOID m_hProj;
  DWORD m_dwProjectFileType;
  CImmCompoundEffect* m_pCreatedEffects;
  CImmDevice* m_pDevice;
  LPDIRECTINPUT m_piDI7;
  LPDIRECTINPUTDEVICE2 m_piDIDevice7;
  TCHAR[260] m_szProjectFileName;
  int m_nCreatedEffects;
  CImmProject* m_pNext;
};

struct combatManager::adjacency_array
{
  int16[6] adjacent;
};

struct combatManager::TArcher
{
  TCreatureType Type;
  CSprite* Sprite;
  CSprite* Missile;
  int X;
  int Y;
  int Facing;
  int Sequence;
  int Frame;
  int Amount;
};

struct SBolt
{
  void sourceX;
  void sourceY;
  void destX;
  void destY;
  void splitFrequency;
  void startThickness;
  void color;
  int dword1C;
  int dword20;
  float float24;
  float float28;
  int source_X;
  void source_Y;
  void out_of_range;
  float dword34;
  float float3C;
  int dword40;
  void someBool;
  void dword44;
  void[12] gap48;
  int dword54;
  int thickness;
  int endThickness;
  int length;
  int angleDistortMin;
  int dword68;
  float dword6C;
  int dword70;
  int dword74;
};

enum spell_flags
{
  SF_BATTLE_SPELL = 0x1,
  SF_MAP_SPELL = 0x2,
  SF_TIME_SCALE = 0x4,
  SF_CREATURE_SPELL = 0x8,
  SF_SINGLE_TARGET = 0x10,
  SF_SINGLE_SHOOTING_STACK = 0x20,
  SF_EXPERT_MASS_VERSION = 0x40,
  SF_TARGET_ANYWHERE = 0x80,
  SF_REMOVE_OBSTACLE = 0x100,
  SF_DAMAGE_SPELL = 0x200,
  SF_MIND_SPELL = 0x400,
  SF_FRIENDLY_MASS = 0x800,
  SF_NOT_AT_WAR_MACHINE = 0x1000,
  SF_SPELL_FROM_ARTIFACT = 0x2000,
  SF_DEFENSIVE = 0x4000,
  SF_AI_DAMAGE_SPELL = 0x8000,
  SF_AI_AREA_EFFECT = 0x10000,
  SF_AI_MASS_DAMAGESPELL = 0x20000,
  SF_AI_NON_DAMAGE_SPELL = 0x40000,
  SF_AI_CREATURES = 0x80000,
  SF_AI_ADVENTUREMAP = 0x100000,
};

struct TSpellTraits
{
  int m_karma;
  int8* m_sample;
  int m_effect;
  uint m_flags;
  int8* m_name;
  int8* m_abbreviated_name;
  int m_level;
  TSpellSchool m_school;
  int[4] m_manaCost;
  int m_power_factor;
  int[4] m_mastery_bonus;
  int[9] m_townGetsItChance;
  int[4] m_AI_value;
  int8*[4] m_description;
};

struct combatManager::TWallTarget
{
  int16 target_hex;
  int16 blocked_row;
  int16 hit_x;
  int16 hit_y;
  combatManager::TWallSection wall;
};

struct TCombatWindow : heroWindow
{
  byte[64] unknown40;
};

struct SAMPLE2
{
  sample* resSample;
  HSAMPLE playSample;
};

struct configStruct
{
  int[2] walkSpeed;
  int musicVolume;
  int soundVolume;
  int lastMusicVolume;
  int lastSoundVolume;
  int AutoSave;
  int ShowRoute;
  int MoveReminder;
  int QuickCombat;
  int VideoSubtitles;
  int TownOutlines;
  int AnimateSpellBook;
  int WindowScrollSpeed;
  int BlackoutComputer;
  int AutoCreatures;
  int AutoSpells;
  int AutoCatapult;
  int AutoBallista;
  int AutoFirstAidTent;
  int PreferBink;
  int MainGameShowMenu;
  int ScreenX;
  int ScreenY;
  int FullScreen;
  int bCombatShowEntireGrid;
  int bCombatShowMouseHex;
  int iCombatGridLevel;
  int[7] iCombatViewArmy;
  int8[2] padding;
  bool bDontTryRedbook;
  bool bFirstInstall;
  int8[4] cUniqueSystemID;
  int iCombatSpeed;
  int unknown;
  int8[13] cCurRemoteReceive;
  int8[13] cRemoteReceiveDiff;
  int8[13] cCurRemoteSend;
  int8[21] cNetName;
};

struct CDiffHeader
{
  int m_numBytes;
  int m_oldNumBytes;
  bool m_copy;
};

struct CDiffFile
{
  BYTE* saveGameSize;
};

enum FileMode
{
  modeRead = 0x80000000,
  modeWrite = 0x140000000,
  modeReadWrite = 0x1C0000000,
};

struct CLogFile
{
  int8[351] m_logFileName;
};

struct CTurnDuration
{
  ulong m_lastWarned;
  unsigned int m_turnStartTime;
  unsigned int m_currDuration;
  unsigned int m_nextWarning;
  ulong m_pauseTime;
};

struct TViewArmyWindow : CAdvPopup
{
  TCreatureType ArmyType;
  int ArmySize;
  int morale;
  std::string morale_help;
  int luck;
  std::string luck_help;
  int Upgrade;
  bool ShowingUpgradeButton;
  bool ShowingDismissButton;
  bool ShowingOkButton;
  SpellID[3] Influence;
  int[3] Duration;
  void* RolloverWidget;
  void* SpriteWidget;
};

struct highScoreManager::HighScoreRec
{
  int8[41] playerName;
  int8[43] scenarioName;
  int totalScore;
  int totalTime;
  int basicScore;
  bool bIsCheater;
};

struct type_AI_attack_hex_chooser
{
  army* attacker;
  int speed;
  army* enemy;
  searchArray* search_array;
  int* enemy_attacks;
  int retaliation_strength;
  int our_strength;
  int best_value;
  int best_hex;
  int best_attack_time;
  type_AI_combat_parameters* estimate;
};

struct generator
{
  int8 genClass;
  int8 genType;
  TCreatureType[4] type;
  int16[4] population;
  armyGroup guards;
  uchar mapX;
  uchar mapY;
  uchar mapZ;
  int8 playerOwner;
  int8 town_id;
};

struct TTimedEvent
{
  std::string Message;
  int[7] ResQty;
  uchar PlayerFlags;
  bool ApplyToPlayer;
  bool ApplyToComputer;
  ushort FirstTime;
  ushort Interval;
};

struct TTownEvent : TTimedEvent
{
  int8 TownNum;
  int64 BuildBuildings;
  ushort[7] generatorBonuses;
};

struct TBlackMarket
{
  TArtifact[7] artifacts;
};

struct Sign
{
  int hasText;
  std::string signText;
};

struct CObjectType
{
  std::string ImageName;
  int8 Width;
  int8 Height;
  std::bitset_48_ PlacementMask;
  std::bitset_48_ PassableMask;
  std::bitset_48_ ShadowMask;
  std::bitset_48_ TriggerMask;
  std::bitset_10_ TerrainMask;
  TAdventureObjectType Type;
  int Subtype;
  int8 IsUnderlay;
  byte align1;
  int16 objectTypeIndex;
};

struct CObject : ExtraInfoUnion
{
  uchar x;
  uchar y;
  uchar z;
  ushort TypeID;
  uchar frameOffset;
};

struct TreasureData
{
  std::string Message;
  bool HasCustomGuardians;
  armyGroup Guardians;
};

struct MonsterData
{
  std::string Message;
  int[7] ResQty;
  TArtifact Artifact;
};

struct BlackBoxData : TreasureData
{
  bool HasCustomTreasure;
  int ExperienceBonus;
  int ManaBonus;
  int8 MoraleBonus;
  int8 LuckBonus;
  int[7] ResQty;
  int8[4] PrimarySkillBonus;
  std::vector SecondarySkills;
  std::vector Artifacts;
  std::vector Spells;
  armyGroup Creatures;
};

struct type_event_record
{
  type_event_record::vftable_t* vftable;
  int8 player_id;
};

struct SecondarySkillData
{
  TSecondarySkill type;
  TSkillMastery level;
};

struct TQuestGuard
{
  type_quest* quest;
  byte setup;
};

struct TSeerHut : TQuestGuard
{
  TSeerReward reward;
  int8 NameIndex;
  byte unknown;
};

struct slider : widget
{
  CSprite* sliderSprite;
  Bitmap816* sliderBitmap;
  int oldState;
  int currentState;
  int knobPos;
  int knobRange;
  int numStates;
  int length;
  int pageSize;
  int knob_start;
  int16 clickX;
  int16 clickY;
  bool hotKeys;
  bool scrolling;
  bool lastFocus;
  byte[5] gap_5F;
  TSliderFunction sliderFunction;
};

struct CCombatInitMsg : t_complex_net_message
{
  type_point m_point;
  uchar m_leftHero;
  uchar m_rightTown;
  uchar m_rightHero;
  byte[1] gap_1F;
  int m_seed;
  int m_winner;
  bool m_retreatWin;
  bool m_combatSurrender;
  byte[2] gap_2A;
  int m_leftOwner;
  int m_leftGold;
  int m_rightOwner;
  int m_rightGold;
  armyGroup m_leftArmyGroup;
  armyGroup m_rightArmyGroup;
  byte[4] gap_AC;
  town m_town;
  hero m_leftHeroData;
  hero m_rightHeroData;
};

struct recruitUnit : baseManager
{
  int[4] CurrentSpriteFrame;
  int type;
  bool view_only;
  TCreatureType monsterType;
  int16* numAvail;
  int selectedPosition;
  TCreatureType MonType1;
  TCreatureType MonType2;
  TCreatureType MonType3;
  TCreatureType MonType4;
  int16*[4] available;
  hero* thisHero;
  int* availSource;
  int goldPerTroop;
  int altResource;
  int resourcesPerTroop;
  int bInTownMainScreen;
  heroWindow* errorWin;
  armyGroup* currArmyGroup;
  bool bCurrArmyGroupIsTownGarrison;
  int addIndex;
  int updateNeeded;
  int errorExit;
  int maxAvail;
  int totalGold;
  int totalResources;
  int numberToBuy;
};

struct TMageGuildWindow { };

struct type_AI_creature_swapper
{
  armyGroup* army;
  armyGroup* adjacent_army;
  bool has_angelic_alliance;
  int16 morale;
  int16 alignment_count;
  int8[10] alignments;
  int army_value_increase;
  int16 improvement;
};

struct type_AI_creature_purchaser : type_AI_creature_swapper
{
  int player_id;
  int* funds;
  bool subtract_cost_mode;
  std::vector_type_creature_source_ creatures;
};

struct type_creature_source
{
  TCreatureType type;
  int16* ptr;
  int16 number;
  bool is_free;
};

struct CDPlayLobby : CDPlay
{
  IDirectPlayLobby3* m_lpLobby;
  CAutoArray_CDPlayAddressElement_* m_pAddressArray;
};

struct DPLCONNECTION
{
  uint dwSize;
  ulong dwFlags;
  DPSESSIONDESC2* lpSessionDesc;
  DPNAME* lpPlayerName;
  GUID guidSP;
  void* lpAddress;
  int dwAddressSize;
};

struct DPNAME
{
  int dwSize;
  int dwFlags;
  int8* lpszShortNameA;
  int8* lpszLongNameA;
};

struct DPSESSIONDESC2
{
  uint dwSize;
  uint dwFlags;
  GUID guidInstance;
  GUID guidApplication;
  uint dwMaxPlayers;
  uint dwCurrentPlayers;
  int8* lpszSessionNameA;
  int8* lpszPasswordA;
  uint dwReserved1;
  uint dwReserved2;
  uint dwUser1;
  uint dwUser2;
  uint dwUser3;
  uint dwUser4;
};

struct DPCAPS
{
  ulong dwSize;
  unsigned int dwFlags;
  unsigned int dwMaxBufferSize;
  unsigned int dwMaxQueueSize;
  unsigned int dwMaxPlayers;
  ulong dwHundredBaud;
  ulong dwLatency;
  ulong dwMaxLocalPlayers;
  ulong dwHeaderLength;
  ulong dwTimeout;
};

struct IDirectPlayLobby2 : IDirectPlayLobby { };

struct CDPlayMsg
{
  uchar* pData;
  ulong dataSize;
};

struct boat : type_obscuring_object
{
  bool allocated;
  uchar id;
  int8 type;
  int8 facing;
  int8 playerOwner;
  THeroID occupying_hero;
  bool occupied;
};

struct type_spellvalue
{
  hero* our_hero;
  int stack_value;
  int power;
  int duration;
  int mana;
  std::vector_type_creature_value_ list;
};

struct _SAMPLE
{
  int8[4] tag;
  HDIGDRIVER driver;
  unsigned int status;
  void*[2] start;
  unsigned int[2] len;
  unsigned int[2] pos;
  unsigned int[2] done;
  int[2] reset_ASI;
  unsigned int src_fract;
  int left_val;
  int right_val;
  int current_buffer;
  int last_buffer;
  int starved;
  int loop_count;
  int loop_start;
  int loop_end;
  int format;
  unsigned int flags;
  int playback_rate;
  float save_volume;
  float save_pan;
  float left_volume;
  float right_volume;
  float wet_level;
  float dry_level;
  LOWPASS_INFO lp;
  int service_type;
  AILSAMPLECB SOB;
  AILSAMPLECB EOB;
  AILSAMPLECB EOS;
  int[8] user_data;
  int[8] system_data;
  ADPCMDATA adpcm;
  int secondary_buffer;
  int service_interval;
  int service_tick;
  int buffer_segment_size;
  int prev_segment;
  int prev_cursor;
  int bytes_remaining;
  int direct_control;
  int doeob;
  int dosob;
  int doeos;
  DPINFO[3] pipeline;
};

struct LOWPASS_UPDATED_INFO
{
  int XL0;
  int XL1;
  int YL0;
  int YL1;
  int XR0;
  int XR1;
  int YR0;
  int YR1;
};

struct LOWPASS_CONSTANT_INFO
{
  int A;
  int B0;
  int B1;
};

struct LOWPASS_INFO
{
  LOWPASS_UPDATED_INFO u;
  LOWPASS_CONSTANT_INFO c;
  float cutoff;
  int on;
};

struct ADPCMDATA
{
  unsigned int blocksize;
  unsigned int extrasamples;
  unsigned int blockleft;
  unsigned int step;
  unsigned int savesrc;
  unsigned int sample;
  unsigned int destend;
  unsigned int srcend;
  unsigned int samplesL;
  unsigned int samplesR;
  unsigned int16[16] moresamples;
};

struct DPINFO
{
  byte[104] off;
};

_DIG_DRIVER*;

void (__stdcall *AILSAMPLECB)(HSAMPLE sample);

struct _DIG_DRIVER
{
  int8[4] tag;
  int backgroundtimer;
  int quiet;
  int n_active_samples;
  float master_volume;
  int DMA_rate;
  int hw_format;
  unsigned int hw_mode_flags;
  int channels_per_sample;
  int bytes_per_channel;
  int channels_per_buffer;
  int samples_per_buffer;
  int playing;
  HSAMPLE samples;
  int n_samples;
  int build_size;
  int* build_buffer;
  int[8] system_data;
  int buffer_size;
  void* hWaveOut;
  unsigned int reset_works;
  unsigned int request_reset;
  LPWAVEHDR first;
  int n_buffers;
  LPWAVEHDR* return_list;
  int return_head;
  int return_tail;
  unsigned int deviceid;
  PCMWAVEFORMAT wformat;
  unsigned int guid;
  void* pDS;
  unsigned int ds_priority;
  int emulated_ds;
  void* lppdsb;
  HWND dsHwnd;
  void** lpbufflist;
  HSAMPLE* samp_list;
  int* sec_format;
  int max_buffs;
  int released;
  unsigned int foreground_timer;
  HDIGDRIVER next;
  int callingCT;
  int callingDS;
  int DS_initialized;
  void* DS_sec_buff;
  void* DS_out_buff;
  int DS_buffer_size;
  int DS_frag_cnt;
  int DS_frag_size;
  int DS_last_frag;
  int DS_last_write;
  int DS_last_timer;
  int DS_skip_time;
  int DS_use_default_format;
  float master_wet;
  float master_dry;
  int use_MMX;
  unsigned int us_count;
  unsigned int ms_count;
  unsigned int last_ms_polled;
  unsigned int last_percent;
  DPINFO[4] pipeline;
  REVERB_INFO ri;
  int* reverb_build_buffer;
  int reverb_build_size;
  int reverb_buffer_size;
  int reverb_on;
  unsigned int reverb_off_time;
  unsigned int reverb_duration;
  float reverb_time;
  float reverb_damping;
  float reverb_predelay;
  unsigned int reverb_into;
  unsigned int reverb_outof;
  int no_wom_done;
  unsigned int wom_done_buffers;
};

struct _WAVEHDR
{
  int dwFlags;
  int dwBytesRecorded;
  int dwUser;
  int temp;
  void* lpData;
  int dwBufferLength;
  int longdwLoops;
  int dwLoops;
  void* lpNext;
  int* reserved;
};

_WAVEHDR*;

struct WAVEFORMAT
{
  WORD wFormatTag;
  WORD nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD nBlockAlign;
};

struct PCMWAVEFORMAT
{
  WAVEFORMAT* wF;
  WORD wBitsPerSample;
};

struct REVERB_CONSTANT_INFO
{
  float* start0;
  float* start1;
  float* start2;
  float* start3;
  float* start4;
  float* start5;
  float* end0;
  float* end1;
  float* end2;
  float* end3;
  float* end4;
  float* end5;
  float C0;
  float C1;
  float C2;
  float C3;
  float C4;
  float C5;
  float A;
  float B0;
  float B1;
};

struct REVERB_UPDATED_INFO
{
  float* address0;
  float* address1;
  float* address2;
  float* address3;
  float* address4;
  float* address5;
  float X0;
  float X1;
  float Y0;
  float Y1;
};

struct REVERB_INFO
{
  REVERB_UPDATED_INFO u;
  REVERB_CONSTANT_INFO c;
};

struct waveformat_tag
{
  WORD wFormatTag;
  WORD nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD nBlockAlign;
};

struct type_record_erase : type_event_record
{
  type_point location;
  int object_id;
  unsigned int object_extra_info;
  int object_index;
};

struct type_record_player_death : type_event_record
{
  int8 died_player;
};

struct type_record_teleport : type_record_move_hero { };

struct type_record_hide_boat : type_event_record
{
  boat* current_boat;
  bool is_occupied;
  bool was_occupied;
  THeroID new_hero_id;
  THeroID prev_hero_id;
};

struct type_record_show_boat : type_record_hide_boat
{
  type_point new_location;
  type_point prev_location;
};

struct type_record_hide_hero : type_event_record
{
  hero* current_hero;
  int8 new_owner;
  int8 prev_owner;
  bool town_garrison;
};

struct type_record_show_hero : type_record_hide_hero
{
  type_point new_location;
  type_point prev_location;
  bool is_in_boat;
  bool was_in_boat;
};

struct type_record_shroud : type_event_record
{
  std::vector_type_shroud_change_ changes;
};

struct type_record_move_hero : type_event_record
{
  hero* current_hero;
  type_point start;
  int8 facing_start;
  int8 facing_end;
  type_point destination;
};

struct type_creature_bank
{
  armyGroup guards;
  int[7] resources;
  TCreatureType reward_creature;
  int8 reward_creatures;
  std::vector_type_creature_source_ artifacts;
};

struct TGzFile : TAbstractFile
{
  gzFile file;
};

struct type_event_record::vftable_t
{
  void* (__thiscall *)(type_event_record* scalar_deleting_destructor, uint this);
  type_event_record_type (__thiscall *)(type_event_record* scalar_deleting_destructor);
  bool (__thiscall *)(type_event_record* scalar_deleting_destructor, TGzFile* this, int flags);
  bool (__thiscall *)(type_event_record* scalar_deleting_destructor, TGzFile* this, int flags);
  void (__thiscall *)(type_event_record* scalar_deleting_destructor, bool this);
  void (__thiscall *)(type_event_record* scalar_deleting_destructor);
};

struct TSubWindow::vftable_t
{
  void* (__thiscall *)(TSubWindow* scalar_deleting_destructor, uint this);
};

struct heroWindow::vftable_t
{
  void* (__thiscall *)(heroWindow* scalar_deleting_destructor, bool Open);
  int (__thiscall *)(heroWindow* scalar_deleting_destructor, int Open, bool Close);
  void (__thiscall *)(heroWindow* scalar_deleting_destructor, bool Open);
  int (__thiscall *)(heroWindow* scalar_deleting_destructor, message* Open);
  void (__thiscall *)(heroWindow* scalar_deleting_destructor, widget* Open);
  void (__thiscall *)(heroWindow* scalar_deleting_destructor, bool Open, int Close, int this);
  void (__thiscall *)(heroWindow* scalar_deleting_destructor, bool Open);
  void (__thiscall *)(heroWindow* scalar_deleting_destructor);
  void (__thiscall *)(heroWindow* scalar_deleting_destructor, bool Open);
};

struct TViewWorldWindow : CAdvPopup
{
  int viewable_width;
  int viewable_height;
  type_func_button* RolloverWidget;
  type_point origin;
};

struct type_func_button : button
{
  int (__fastcall *)(message* handler);
};

struct garrison
{
  int8 playerOwner;
  armyGroup garrisonArmy;
  bool armyRemovable;
  uchar mapX;
  uchar mapY;
  uchar mapZ;
};

struct t_campaign_type_window : heroWindow { };

struct TCampaignWindow : heroWindow { };

struct CChatManager
{
  CChatManager::CChatStr* msgArray;
  int currMsg;
  int msgCount;
  int8* widgetText;
  uint pauseTime;
  bool changed;
  textWidget* lastWidget;
  int maxLines;
  int position;
  bool chatKilled;
  uint channel;
  bool isSysMsg;
  sample* chatSample;
  sample* playerDropSample;
  sample* sysMsgSample;
  sample* turnDurSample;
  sample* playerEnterSample;
};

struct CChatManager::CChatStr
{
  int8[128] sText;
  ulong killTime;
  bool isSystem;
};

struct TThievesGuildWindow { };

struct CGameTransferDlg : CTextDialog
{
  CGameTransferSmack smack;
  bool m_sending;
};

struct CTextDialog : TDialogBox
{
  textWidget* pTextWidget;
};

struct CAnimatedDlg : CTextDialog
{
  uint m_lastTick;
  int m_spriteX;
  int m_spriteY;
  int m_spriteFrame;
  int m_seq;
  int8* m_sSprite;
  bool m_palUpdated;
  CSprite* m_pSprite;
};

struct CGameTransferSmack
{
  int m_x;
  int m_y;
  int m_lastFrame;
  bool m_started;
  bool m_sending;
  bool m_drawText;
  CSaveScreen* m_saveScreen;
};

struct TSpellEffectTraits
{
  int8* m_spriteName;
  int8* m_name;
  FSpellEffectFlags m_flags;
};

typedef EXCEPTION_RECORD _EXCEPTION_RECORD;

struct _EXCEPTION_RECORD
{
  DWORD ExceptionCode;
  DWORD ExceptionFlags;
  _EXCEPTION_RECORD* ExceptionRecord;
  PVOID ExceptionAddress;
  DWORD NumberParameters;
  ULONG_PTR[15] ExceptionInformation;
};

void*;

struct combatManager::TObstacleInfo
{
  ulong backgroundMask;
  uchar height;
  uchar width;
  uchar numSquares;
  uchar underlay;
  int8[8] sOffsets;
  int8* FileName;
};

enum TSpellEffectID
{
  eSpellEffectNone = 0xFFFFFFFF,
  eSpellEffectPrayer = 0x100000000,
  eSpellEffectLightning_Bolt = 0x100000001,
  eSpellEffectAirShield = 0x100000002,
  eSpellEffectBacklash = 0x100000003,
  eSpellEffectAnimateDead = 0x100000004,
  eSpellEffectAntiMagic = 0x100000005,
  eSpellEffectBlind = 0x100000006,
  eSpellEffectCounterstroke = 0x100000007,
  eSpellEffectDeathRipple = 0x100000008,
  eSpellEffectFireblast = 0x100000009,
  eSpellEffectDecay = 0x10000000A,
  eSpellEffectFireShield = 0x10000000B,
  eSpellEffectFirestorm = 0x10000000C,
  eSpellEffectDisruptiveRay_Ray = 0x10000000D,
  eSpellEffectDisruptiveRay_Burst = 0x10000000E,
  eSpellEffectFear = 0x10000000F,
  eSpellEffectMeteorShower = 0x100000010,
  eSpellEffectFrenzy = 0x100000011,
  eSpellEffectFortune = 0x100000012,
  eSpellEffectMuckAndMire = 0x100000013,
  eSpellEffectMirth = 0x100000014,
  eSpellEffectHypnotize = 0x100000015,
  eSpellEffectProtectionFromAir = 0x100000016,
  eSpellEffectProtectionFromWater = 0x100000017,
  eSpellEffectProtectionFromFire = 0x100000018,
  eSpellEffectPrecision = 0x100000019,
  eSpellEffectProtectionFromEarth = 0x10000001A,
  eSpellEffectShield = 0x10000001B,
  eSpellEffectSlayer = 0x10000001C,
  eSpellEffectSacredBreath = 0x10000001D,
  eSpellEffectSorrow = 0x10000001E,
  eSpellEffectTailWind = 0x10000001F,
  eSpellEffectForcefield_2 = 0x100000020,
  eSpellEffectForcefield_3 = 0x100000021,
  eSpellEffectRemoveObstacle = 0x100000022,
  eSpellEffectBerserk = 0x100000023,
  eSpellEffectBless = 0x100000024,
  eSpellEffectChainLightning_Bolt = 0x100000025,
  eSpellEffectChainLightning_Dust = 0x100000026,
  eSpellEffectCure = 0x100000027,
  eSpellEffectCurse = 0x100000028,
  eSpellEffectDispel = 0x100000029,
  eSpellEffectForgetfulness = 0x10000002A,
  eSpellEffectFirewall_2 = 0x10000002B,
  eSpellEffectFirewall_3 = 0x10000002C,
  eSpellEffectFrostRing = 0x10000002D,
  eSpellEffectIceRay_Burst = 0x10000002E,
  eSpellEffectLandMine = 0x10000002F,
  eSpellEffectMisfortune = 0x100000030,
  eSpellEffectLightning_Dust = 0x100000031,
  eSpellEffectResurrection = 0x100000032,
  eSpellEffectSacrifice_Slay = 0x100000033,
  eSpellEffectSacrifice_Resurrect = 0x100000034,
  eSpellEffectSpontaneousCombustion = 0x100000035,
  eSpellEffectToughSkin = 0x100000036,
  eSpellEffectQuicksand = 0x100000037,
  eSpellEffectWeakness = 0x100000038,
  eSpellEffectLandMineExplosion = 0x100000039,
  eSpellEffectDispelQuicksand = 0x10000003A,
  eSpellEffectDispelLandMine = 0x10000003B,
  eSpellEffectDispelForcefield_2 = 0x10000003C,
  eSpellEffectDispelForcefield_3 = 0x10000003D,
  eSpellEffectDispelFirewall_2 = 0x10000003E,
  eSpellEffectDispelFirewall_3 = 0x10000003F,
  eSpellEffectMagicBolt_Burst = 0x100000040,
  eSpellEffectFirewall_1 = 0x100000041,
  eSpellEffectDispelFirewall_1 = 0x100000042,
  eSpellEffectPoison = 0x100000043,
  eSpellEffectBind = 0x100000044,
  eSpellEffectDisease = 0x100000045,
  eSpellEffectParalyze = 0x100000046,
  eSpellEffectAge = 0x100000047,
  eSpellEffectDeathCloud = 0x100000048,
  eSpellEffectDeathBlow = 0x100000049,
  eSpellEffectDrainLife = 0x10000004A,
  eSpellEffectMagicChannel_Suck = 0x10000004B,
  eSpellEffectMagicChannel_Spew = 0x10000004C,
  eSpellEffectMagicDrain = 0x10000004D,
  eSpellEffectMagicResistance = 0x10000004E,
  eSpellEffectRegenerate = 0x10000004F,
  eSpellEffectDeathStare = 0x100000050,
  eSpellEffectPoof = 0x100000051,
  kNumSpellEffects = 0x100000052,
};

struct combatManager::TObstacle
{
  CSprite* sprite;
  combatManager::TObstacleInfo* info;
  uchar grid_index;
  int8 owner;
  bool is_visible;
  int damage;
  int duration;
  TSpellEffectID dispel_effect;
};

struct std::vector_combatManager::TObstacle_
{
  int8 allocator;
  combatManager::TObstacle* first;
  combatManager::TObstacle* last;
  combatManager::TObstacle* end;
};

struct std::vector_pathCell_ptr_
{
  int8 allocator;
  pathCell** first;
  pathCell** last;
  pathCell** end;
};

struct std::vector_pathCell_
{
  int8 allocator;
  pathCell* first;
  pathCell* last;
  pathCell* end;
};

struct std::vector_army_ptr_
{
  int8 allocator;
  army** first;
  army** last;
  army** end;
};

struct std::vector_resource_ptr_
{
  int8 allocator;
  resource** first;
  resource** last;
  resource** end;
};

struct std::vector_widget_ptr_
{
  int8 allocator;
  widget** first;
  widget** last;
  widget** end;
};

typedef TWidgetVector std::vector_widget_ptr_;

struct type_creature_value
{
  TCreatureType type;
  int value;
  int16 amount;
};

struct std::vector_type_creature_value_
{
  int8 allocator;
  type_creature_value* first;
  type_creature_value* last;
  type_creature_value* end;
};

struct std::vector_type_creature_source_
{
  int8 allocator;
  type_creature_source* first;
  type_creature_source* last;
  type_creature_source* end;
};

struct std::vector_type_creature_bank_
{
  int8 allocator;
  type_creature_bank* first;
  type_creature_bank* last;
  type_creature_bank* end;
};

struct CScenarioInfoDlg : CAdvPopup
{
  CSprite* VictoryIcon;
  CSprite* LossIcon;
  CSprite* TownPix;
  CSprite* bonusSprite;
  Bitmap816*[8] Panels;
  Bitmap816*[8] Flags;
  CSprite* heroSpecificAbility;
};

struct TAbstractFile::vftable_t
{
  void (__thiscall *)(TAbstractFile* scalar_deleting_destructor, uint this);
  int (__thiscall *)(TAbstractFile* scalar_deleting_destructor, void* this, uint flag);
  int (__thiscall *)(TAbstractFile* scalar_deleting_destructor, void* this, uint flag);
};

struct std::vector_generator_
{
  int8 allocator;
  generator* first;
  generator* last;
  generator* end;
};

struct std::vector_TBlackMarket_
{
  int8 allocator;
  TBlackMarket* first;
  TBlackMarket* last;
  TBlackMarket* end;
};

struct game::TRumour
{
  std::string RumourText;
  bool Unavailable;
};

struct mine
{
  int8 playerOwner;
  int8 type;
  bool is_abandoned;
  armyGroup guards;
  unsigned int8 mapX;
  unsigned int8 mapY;
  unsigned int8 mapZ;
};

struct type_record_claim_mine : type_event_record
{
  int id;
  int8 new_owner;
  int8 prev_owner;
};

struct CMapChange : CNetMsg { };

struct TCheatCode
{
  int8[200] code;
};

struct TDrawParts
{
  bool IsValid;
  int X;
  int Y;
  int Id;
};

struct TTavernWindow : CAdvPopup { };

struct CNewMapHeaderInfoMsg : t_complex_net_message
{
  NewSMapHeader m_mapHeader;
};

struct type_monster { };

struct type_sacrifice_window : CAdvPopup
{
  hero* current_hero;
  type_artifact_offering holding_artifact;
  bool sacrificing_artifacts;
  bool can_sacrifice_artifacts;
  bool can_sacrifice_creatures;
  int total_experience;
  textWidget* experience_widget;
  textWidget* experience_total_widget;
  textWidget* current_artifact_value;
  textWidget* creature_name_widget;
  textWidget* rollover_widget;
  iconWidget* current_artifact_widget;
  slider* creature_slider;
  type_func_button* left_backpack_button;
  type_func_button* right_backpack_button;
  type_func_button* empty_backpack_button;
  type_func_button* sacrifice_button;
  type_func_button* all_artifacts_button;
  type_func_button* creatures_button;
  type_func_button* max_creatures_button;
  type_func_button* all_creatures_button;
  type_func_button* artifacts_button;
  std::vector_type_artifact_offering_ artifact_offerings;
  std::vector_widget_ptr_ artifact_value_widgets;
  std::vector_widget_ptr_ artifact_offering_widgets;
  std::vector_widget_ptr_ slot_back_widgets;
  std::vector_widget_ptr_ slot_widgets;
  std::vector_widget_ptr_ backpack_widgets;
  int[8][7] creature_offerings;
  int[8] current_creature;
  std::vector_widget_ptr_ artifact_widgets;
  std::vector_widget_ptr_ creature_widgets;
  bool artifact_mode;
};

enum hero_flags
{
  HF_WELL = 0x1,
  HF_STABLES = 0x2,
  HF_BUOY = 0x4,
  HF_SWANPOND = 0x8,
  HF_IDOLFORTUNEMORALE = 0x10,
  HF_FOUNTAINFORTUNE_N1 = 0x20,
  HF_WATERINGHOLE = 0x40,
  HF_OASIS = 0x80,
  HF_TEMPLE = 0x100,
  HF_SHIPWRECK = 0x200,
  HF_CRYPT = 0x400,
  HF_DERELICTSHIPPENALTY = 0x800,
  HF_PYRAMID = 0x1000,
  HF_FAERIERING = 0x2000,
  HF_FOUNTAINOFYOUTH = 0x4000,
  HF_MERMAIDS = 0x8000,
  HF_RALLYFLAG = 0x10000,
  HF_ISINTAVERN = 0x20000,
  HF_ISINBOAT = 0x40000,
  HF_UNKNOWN = 0x80000,
  HF_SIRENS = 0x100000,
  HF_WARRIORTOMB = 0x200000,
  HF_LUCKCHEAT = 0x400000,
  HF_MORALECHEAT = 0x800000,
  HF_MOVEPOINTSCHEAT = 0x1000000,
  HF_IDOLFORTUNELUCK = 0x2000000,
  HF_TEMPLEDAY7 = 0x4000000,
  HF_FOUNTAINFORTUNE_1 = 0x8000000,
  HF_FOUNTAINFORTUNE_2 = 0x10000000,
  HF_FOUNTAINFORTUNE_3 = 0x20000000,
  HF_UNKNOWN2 = 0x40000000,
};

struct TQuickTownWindow { };

struct TownExtra
{
  int objRef;
  int8 playerOwner;
  bool bCustomBuildings;
  int64 BuildingBuiltMask;
  int64 BuildingDisabledMask;
  bool HasFort;
  bool bCustomArmies;
  armyGroup townArmy;
  bool bCustomName;
  std::string cName;
  int townType;
  bool bIsGrouped;
  byte[3] align1;
  std::bitset_70_ SpellDisabledMask;
  std::bitset_70_ SpellMask;
};

struct TCampaignBrief : heroWindow
{
  ushort* zBuffer;
  int oldVolume;
  std::vector_CampaignScenarioPreview_ scenarios;
  TCampaignBrief::CampaignHeaderStruct* campaign;
  int unknown;
  int selected_scenario;
  coloredBorderFrame*[3] start_bonus_borders;
  bitmapBorder*[3] bitmap_bonus_images;
  iconWidget*[3] sprite_bonus_images;
  button*[5] difficulty_buttons;
  type_func_button* difficulty_decr_button;
  type_func_button* difficulty_incr_button;
  type_text_scroller* scroller;
};

struct IDirectDrawSurface4
{
  IDirectDraw::IDirectDrawVtbl* vftable;
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwHeight;
  DWORD dwWidth;
  LONG lPitch;
  DWORD dwBackBufferCount;
  DWORD dwAlphaBitDepth;
  DWORD dwReserved;
  LPVOID lpSurface;
  DDCOLORKEY ddckCKDestOverlay;
  DDCOLORKEY ddckCKDestBlt;
  DDCOLORKEY ddckCKSrcOverlay;
  DDCOLORKEY ddckCKSrcBlt;
  DDPIXELFORMAT ddpfPixelFormat;
  DWORD dwTextureStage;
};

struct Bitmap24Bit : resource
{
  int DataSize;
  int ImageSize;
  int Width;
  int Height;
  int* data;
};

struct IDirectDraw
{
  IDirectDraw::IDirectDrawVtbl* lpVtbl;
};

union $F9D0D49E746EA05C6F8F62A8D439C7A9
{
  LONG lPitch;
  DWORD dwLinearSize;
};

union $732C1078520B5FCBD2DC52BA2F31A7C8
{
  DWORD dwMipMapCount;
  DWORD dwZBufferBitDepth;
  DWORD dwRefreshRate;
};

struct DDSCAPS
{
  DWORD dwCaps;
};

struct DDCOLORKEY
{
  DWORD dwColorSpaceLowValue;
  DWORD dwColorSpaceHighValue;
};

union DDSURFACEDESC::$F9D0D49E746EA05C6F8F62A8D439C7A9
{
  LONG lPitch;
  DWORD dwLinearSize;
};

union DDSURFACEDESC::$732C1078520B5FCBD2DC52BA2F31A7C8
{
  DWORD dwMipMapCount;
  DWORD dwZBufferBitDepth;
  DWORD dwRefreshRate;
};

struct DDSURFACEDESC
{
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwHeight;
  DWORD dwWidth;
  DDSURFACEDESC::$F9D0D49E746EA05C6F8F62A8D439C7A9 dwBackBufferCount;
  DWORD dwAlphaBitDepth;
  DDSURFACEDESC::$732C1078520B5FCBD2DC52BA2F31A7C8 dwReserved;
  DWORD lpSurface;
  DWORD ddckCKDestOverlay;
  LPVOID ddckCKDestBlt;
  DDCOLORKEY ddckCKSrcOverlay;
  DDCOLORKEY ddckCKSrcBlt;
  DDCOLORKEY ddpfPixelFormat;
  DDCOLORKEY ddsCaps;
  DDPIXELFORMAT ;
  DDSCAPS ;
};

typedef DDPIXELFORMAT _DDPIXELFORMAT;

struct _DDPIXELFORMAT
{
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwFourCC;
  _DDPIXELFORMAT::$F1D3FB4D78950D0942225445130999CB ;
  _DDPIXELFORMAT::$6A86D2BA2D533C5D3D5AB1F1491969D5 ;
  _DDPIXELFORMAT::$95F56DB01BB1548DF390D9ACB4F5DA09 ;
  _DDPIXELFORMAT::$4C86B66084EB9B6F3AE81991D3FADB38 ;
  _DDPIXELFORMAT::$23DF69239FC04D9BE22118E1AD8451FB ;
};

union _DDPIXELFORMAT::$F1D3FB4D78950D0942225445130999CB
{
  DWORD dwRGBBitCount;
  DWORD dwYUVBitCount;
  DWORD dwZBufferBitDepth;
  DWORD dwAlphaBitDepth;
  DWORD dwLuminanceBitCount;
  DWORD dwBumpBitCount;
  DWORD dwPrivateFormatBitCount;
};

union _DDPIXELFORMAT::$6A86D2BA2D533C5D3D5AB1F1491969D5
{
  DWORD dwRBitMask;
  DWORD dwYBitMask;
  DWORD dwStencilBitDepth;
  DWORD dwLuminanceBitMask;
  DWORD dwBumpDuBitMask;
  DWORD dwOperations;
};

union _DDPIXELFORMAT::$95F56DB01BB1548DF390D9ACB4F5DA09
{
  DWORD dwGBitMask;
  DWORD dwUBitMask;
  DWORD dwZBitMask;
  DWORD dwBumpDvBitMask;
  _DDPIXELFORMAT::$95F56DB01BB1548DF390D9ACB4F5DA09::$A78036EB239B85FA27F661E6E98FFEA9 MultiSampleCaps;
};

struct _DDPIXELFORMAT::$95F56DB01BB1548DF390D9ACB4F5DA09::$A78036EB239B85FA27F661E6E98FFEA9
{
  WORD wFlipMSTypes;
  WORD wBltMSTypes;
};

union _DDPIXELFORMAT::$4C86B66084EB9B6F3AE81991D3FADB38
{
  DWORD dwBBitMask;
  DWORD dwVBitMask;
  DWORD dwStencilBitMask;
  DWORD dwBumpLuminanceBitMask;
};

union _DDPIXELFORMAT::$23DF69239FC04D9BE22118E1AD8451FB
{
  DWORD dwRGBAlphaBitMask;
  DWORD dwYUVAlphaBitMask;
  DWORD dwLuminanceAlphaBitMask;
  DWORD dwRGBZBitMask;
  DWORD dwYUVZBitMask;
};

struct IDirectDraw::IDirectDrawVtbl
{
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, LPRECT this, LPDIRECTDRAWSURFACE4* a1, LPRECT a2, DWORD AddRef, _DDBLTFX* this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1, int a2);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1, int a2, int AddRef, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, DWORD this, LPDIRECTDRAWSURFACE4 a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1, int a2);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, LPRECT this, LPDDSURFACEDESC a1, DWORD a2, HANDLE AddRef);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, LPDIRECTDRAWCLIPPER this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, DWORD this, DDCOLORKEY* a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1, int a2, int AddRef, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1, int a2, int AddRef);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this, int a1, int a2);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface, int this);
  HRESULT (__stdcall *)(IDirectDrawSurface4* QueryInterface);
};

struct CAutoArray
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct CAutoArray::vftable_t
{
  void (__thiscall *)(CAutoArray* scalar_deleting_destructor, uchar this);
  bool (__thiscall *)(CAutoArray* scalar_deleting_destructor, void* this);
  void* (__thiscall *)(CAutoArray* scalar_deleting_destructor, DWORD this);
  bool (__thiscall *)(CAutoArray* scalar_deleting_destructor, DWORD this, void* flag);
  bool (__thiscall *)(CAutoArray* scalar_deleting_destructor, DWORD this);
  bool (__thiscall *)(CAutoArray* scalar_deleting_destructor, DWORD this, void* flag);
  int (__thiscall *)(CAutoArray* scalar_deleting_destructor);
};

struct TOverviewWindow { };

enum GlobalInfoFlags
{
  BuoyInfo = 0x0,
  CloverFieldInfo = 0x1,
  FaerieRingInfo = 0x2,
  FountainOfFortuneInfo = 0x3,
  GardenOfRevelationInfo = 0x4,
  TrainingGroundsInfo = 0x5,
  LibraryInfo = 0x6,
  DefenseTowerInfo = 0x7,
  MercCampInfo = 0x8,
  MagicSchoolInfo = 0x9,
  WarSchoolInfo = 0xA,
  PowerSchoolInfo = 0xB,
  WitchHutInfo = 0xC,
  FountainOfYouthInfo = 0xD,
  HillFortInfo = 0xE,
  MagicSpringInfo = 0xF,
  MermaidInfo = 0x10,
  RallyFlagInfo = 0x11,
  TreeOfKnowledgeInfo = 0x12,
  Shrine1Info = 0x13,
  Shrine2Info = 0x14,
  Shrine3Info = 0x15,
  IdolOfFortuneInfo = 0x16,
  TempleInfo = 0x17,
  UniversityInfo = 0x18,
  MagicWellInfo = 0x19,
  OasisInfo = 0x1A,
  WateringHoleInfo = 0x1B,
  const_sacrifice_info = 0x1C,
  MaxInfoFlags = 0x20,
};

struct type_university
{
  int[4] skills;
};

struct TSpellbookWindow : CAdvPopup
{
  int* AllowedContext;
  hero* Hero;
  armyGroup* EnemyGroup;
  int PlainsType;
  TSpellSchool School;
  uint ContextMask;
  int Page;
  SpellID[6] SpellMap;
  iconWidget** SpellLevelWidgets;
  iconWidget** SpellIconWidgets;
  textWidget** SpellNameWidgets;
  iconWidget* HeadingWidget;
  bitmapBorder* NextPageWidget;
  bitmapBorder* PreviousPageWidget;
  iconWidget* SchoolTabsWidget;
  void* RolloverWidget;
};

struct type_cell_adjuster
{
  hero* obscuring_hero;
  boat* obscuring_boat;
  hero* mobile_hero;
};

enum combatManager::anonymous_enum
{
};

struct THeroScreenWindow : CAdvPopup
{
  int heroLocatorIndex;
  widget* heroLocatorWidget;
};

struct TSkuttleBoatWindow { };

struct type_record_claim_town : type_record_claim_mine { };

struct CImmCompoundEffect
{
  GENERIC_EFFECT_PTR* m_paEffects;
  int m_nEffects;
  int8* m_lpszName;
  GUID m_objID;
  GUID* m_pContainedObjIDs;
  CImmCompoundEffect* m_pNext;
};

struct std::bad_alloc : std::exception { };

struct type_dialog_icon
{
  EGameResource resource;
  int qualifier;
  std::string spriteName;
  std::string text;
  int spriteFrameIndex;
  POINT spritePos;
  int spriteHeight;
  int spriteWidth;
  POINT textPos;
  int textHeight;
  int textWidth;
};

struct coloredBorderFrame : border
{
  int color;
  bool colorize;
};

struct TBottomViewKingdom { };

struct TPuzzleWindow { };

struct CHourGlass
{
  bool m_thread;
};

struct type_garrison_base_window : CAdvPopup
{
  hero* thisHero;
  int lastHover;
  int lastQualifier;
  bool is_join_dialog;
};

struct type_monster_join_window : type_garrison_base_window { };

struct TShipWindow { };

struct type_university_skill_button : iconWidget
{
  bool click;
  TSecondarySkill skill;
};

struct TTradeResourceWindow { };

struct TSellCreatureWindow { };

struct TBuyArtifactWindow { };

struct TSellArtifactWindow { };

struct type_university_window : CAdvPopup
{
  hero* current_hero;
  type_func_button* purchase_button;
  textWidget* purchase_title_widget;
  textWidget* purchase_text_widget;
  textWidget* rollover_widget;
  type_university_skill[4] skills;
  type_university_skill selected_skill;
  TWidgetVector selection_widgets;
  TWidgetVector purchase_widgets;
};

struct TQuickHeroWindow { };

struct TSystemOptionsWindow : CAdvPopup
{
  bool bPrefsChanged;
  int quickCombatSave;
};

struct TDimensionDoorWindow { };

struct TTownGateWindow : CAdvPopup
{
  std::vector_int_ Towns;
  int topTown;
  int selectedTown;
  bool adventure_spell;
};

struct type_garrison_purchaser : type_town_threat_checker { };

struct TSplitWindow : CAdvPopup { };

struct std::bad_cast : std::exception { };

struct TCombatOptionsWindow : heroWindow
{
  bool bPrefsChanged;
  textWidget* RolloverWidget;
};

struct TCombatResultsWindow : heroWindow { };

struct type_creature_bank_level
{
  armyGroup guards;
  EGameResource[7] resources;
  TCreatureType creature_type;
  int8 creature_amount;
  int8 chance;
  int8 upg_chance;
  int8 treasure_artifacts;
  int8 minor_artifacts;
  int8 major_artifacts;
  int8 relic_artifacts;
};

struct TRecruitWindow { };

struct type_artifact_offering_widget : iconWidget
{
  int down_click;
};

struct CSingleSelPopup : TDialogBox { };

struct iconBackedTextWidget { };

struct std::length_error : std::logic_error { };

struct CGiftMsg : CNetMsg
{
  int m_niceGuy;
  int m_resource;
  int m_qty;
};

void* (__cdecl *RADMEMALLOC)(unsigned int32);

void (__cdecl *RADMEMFREE)(void*);

struct BINK
{
  unsigned int32 Width;
  unsigned int32 Height;
  unsigned int32 Frames;
  unsigned int32 FrameNum;
  unsigned int32 LastFrameNum;
  unsigned int32 FrameRate;
  unsigned int32 FrameRateDiv;
  unsigned int32 ReadError;
  unsigned int32 OpenFlags;
  unsigned int32 BinkType;
  unsigned int32 Size;
  unsigned int32 FrameSize;
  unsigned int32 SndSize;
  BINKRECT[8] FrameRects;
  int32 NumRects;
  unsigned int32 PlaneNum;
  void*[2] YPlane;
  void*[2] APlane;
  unsigned int32 YWidth;
  unsigned int32 YHeight;
  unsigned int32 UVWidth;
  unsigned int32 UVHeight;
  void* MaskPlane;
  unsigned int32 MaskPitch;
  unsigned int32 MaskLength;
  unsigned int32 LargestFrameSize;
  unsigned int32 InternalFrames;
  int32 NumTracks;
  unsigned int32 Highest1SecRate;
  unsigned int32 Highest1SecFrame;
  int32 Paused;
  unsigned int32 BackgroundThread;
  void* compframe;
  void* preloadptr;
  unsigned int32* frameoffsets;
  BINKIO bio;
  unsigned int8* ioptr;
  unsigned int32 iosize;
  unsigned int32 decompwidth;
  unsigned int32 decompheight;
  int32* trackindexes;
  unsigned int32* tracksizes;
  unsigned int32* tracktypes;
  int32* trackIDs;
  unsigned int32 numrects;
  unsigned int32 playedframes;
  unsigned int32 firstframetime;
  unsigned int32 startframetime;
  unsigned int32 startblittime;
  unsigned int32 startsynctime;
  unsigned int32 startsyncframe;
  unsigned int32 twoframestime;
  unsigned int32 entireframetime;
  unsigned int32 slowestframetime;
  unsigned int32 slowestframe;
  unsigned int32 slowest2frametime;
  unsigned int32 slowest2frame;
  unsigned int32 soundon;
  unsigned int32 videoon;
  unsigned int32 totalmem;
  unsigned int32 timevdecomp;
  unsigned int32 timeadecomp;
  unsigned int32 timeblit;
  unsigned int32 timeopen;
  unsigned int32 fileframerate;
  unsigned int32 fileframeratediv;
  unsigned int32 runtimeframes;
  unsigned int32 runtimemoveamt;
  unsigned int32* rtframetimes;
  unsigned int32* rtadecomptimes;
  unsigned int32* rtvdecomptimes;
  unsigned int32* rtblittimes;
  unsigned int32* rtreadtimes;
  unsigned int32* rtidlereadtimes;
  unsigned int32* rtthreadreadtimes;
  unsigned int32 lastblitflags;
  unsigned int32 lastdecompframe;
  unsigned int32 lastresynctime;
  unsigned int32 doresync;
  unsigned int32 playingtracks;
  unsigned int32 soundskips;
  BINKSND* bsnd;
  unsigned int32 skippedlastblit;
  unsigned int32 skipped_this_frame;
  unsigned int32 skippedblits;
  BUNDLEPOINTERS bunp;
  unsigned int32 skipped_in_a_row;
  unsigned int32 big_sound_skip_adj;
  unsigned int32 big_sound_skip_reduce;
  unsigned int32 last_time_almost_empty;
  unsigned int32 last_read_count;
  unsigned int32 last_sound_count;
  unsigned int32[16] snd_callback_buffer;
};

BINK*;

struct BINKIO
{
  BINKIOREADHEADER ReadHeader;
  BINKIOREADFRAME ReadFrame;
  BINKIOGETBUFFERSIZE GetBufferSize;
  BINKIOSETINFO SetInfo;
  BINKIOIDLE Idle;
  BINKIOCLOSE Close;
  HBINK bink;
  unsigned int32 ReadError;
  unsigned int32 DoingARead;
  unsigned int32 BytesRead;
  unsigned int32 Working;
  unsigned int32 TotalTime;
  unsigned int32 ForegroundTime;
  unsigned int32 IdleTime;
  unsigned int32 ThreadTime;
  unsigned int32 BufSize;
  unsigned int32 BufHighUsed;
  unsigned int32 CurBufSize;
  unsigned int32 CurBufUsed;
  unsigned int8[160] iodata;
  BINKCBSUSPEND suspend_callback;
  BINKCBTRYSUSPEND try_suspend_callback;
  BINKCBRESUME resume_callback;
  BINKCBIDLE idle_on_callback;
  unsigned int32[16] callback_control;
};

int32 (__cdecl *BINKIOOPEN)(BINKIO*, int8*, unsigned int32);

unsigned int32 (__cdecl *BINKIOREADHEADER)(BINKIO*, int32, void*, unsigned int32);

unsigned int32 (__cdecl *BINKIOREADFRAME)(BINKIO*, unsigned int32, int32, void*, unsigned int32);

unsigned int32 (__cdecl *BINKIOGETBUFFERSIZE)(BINKIO*, unsigned int32);

void (__cdecl *BINKIOSETINFO)(BINKIO*, void*, unsigned int32, unsigned int32, unsigned int32);

unsigned int32 (__cdecl *BINKIOIDLE)(BINKIO*);

void (__cdecl *BINKIOCLOSE)(BINKIO*);

void (__cdecl *BINKCBSUSPEND)(BINKIO*);

int32 (__cdecl *BINKCBTRYSUSPEND)(BINKIO*);

void (__cdecl *BINKCBRESUME)(BINKIO*);

void (__cdecl *BINKCBIDLE)(BINKIO*);

struct BINKSND
{
  BINKSNDREADY Ready;
  BINKSNDLOCK Lock;
  BINKSNDUNLOCK Unlock;
  BINKSNDVOLUME Volume;
  BINKSNDPAN Pan;
  BINKSNDPAUSE Pause;
  BINKSNDONOFF SetOnOff;
  BINKSNDCLOSE Close;
  BINKSNDMIXBINS MixBins;
  BINKSNDMIXBINVOLS MixBinVols;
  unsigned int32 sndbufsize;
  unsigned int8* sndbuf;
  unsigned int8* sndend;
  unsigned int8* sndwritepos;
  unsigned int8* sndreadpos;
  unsigned int32 sndcomp;
  unsigned int32 sndamt;
  unsigned int32 sndconvert8;
  unsigned int32 sndendframe;
  unsigned int32 sndprime;
  unsigned int32 sndpad;
  unsigned int32 BestSizeIn16;
  unsigned int32 BestSizeMask;
  unsigned int32 SoundDroppedOut;
  int32 OnOff;
  unsigned int32 Latency;
  unsigned int32 VideoScale;
  unsigned int32 freq;
  int32 bits;
  int32 chans;
  unsigned int8[256] snddata;
};

int32 (__cdecl *BINKSNDOPEN)(BINKSND*, unsigned int32, int32, int32, unsigned int32, HBINK);

int32 (__cdecl *BINKSNDREADY)(BINKSND*);

int32 (__cdecl *BINKSNDLOCK)(BINKSND*, unsigned int8**, unsigned int32*);

int32 (__cdecl *BINKSNDUNLOCK)(BINKSND*, unsigned int32);

void (__cdecl *BINKSNDVOLUME)(BINKSND*, int32);

void (__cdecl *BINKSNDPAN)(BINKSND*, int32);

void (__cdecl *BINKSNDMIXBINS)(BINKSND*, unsigned int32*, unsigned int32);

void (__cdecl *BINKSNDMIXBINVOLS)(BINKSND*, unsigned int32*, int32*, unsigned int32);

int32 (__cdecl *BINKSNDONOFF)(BINKSND*, int32);

int32 (__cdecl *BINKSNDPAUSE)(BINKSND*, int32);

void (__cdecl *BINKSNDCLOSE)(BINKSND*);

BINKSNDOPEN (__cdecl *BINKSNDSYSOPEN)(unsigned int32);

struct BINKRECT
{
  int32 Left;
  int32 Top;
  int32 Width;
  int32 Height;
};

struct BUNDLEPOINTERS
{
  void* typeptr;
  void* type16ptr;
  void* colorptr;
  void* bits2ptr;
  void* motionXptr;
  void* motionYptr;
  void* dctptr;
  void* mdctptr;
  void* patptr;
};

struct BINKSUMMARY
{
  unsigned int32 Width;
  unsigned int32 Height;
  unsigned int32 TotalTime;
  unsigned int32 FileFrameRate;
  unsigned int32 FileFrameRateDiv;
  unsigned int32 FrameRate;
  unsigned int32 FrameRateDiv;
  unsigned int32 TotalOpenTime;
  unsigned int32 TotalFrames;
  unsigned int32 TotalPlayedFrames;
  unsigned int32 SkippedFrames;
  unsigned int32 SoundSkips;
  unsigned int32 TotalBlitTime;
  unsigned int32 TotalReadTime;
  unsigned int32 TotalDecompTime;
  unsigned int32 TotalBackReadTime;
  unsigned int32 TotalReadSpeed;
  unsigned int32 SlowestFrameTime;
  unsigned int32 Slowest2FrameTime;
  unsigned int32 SlowestFrameNum;
  unsigned int32 Slowest2FrameNum;
  unsigned int32 AverageDataRate;
  unsigned int32 AverageFrameSize;
  unsigned int32 HighestMemAmount;
  unsigned int32 TotalIOMemory;
  unsigned int32 HighestIOUsed;
  unsigned int32 Highest1SecRate;
  unsigned int32 Highest1SecFrame;
};

struct BINKREALTIME
{
  unsigned int32 FrameNum;
  unsigned int32 FrameRate;
  unsigned int32 FrameRateDiv;
  unsigned int32 Frames;
  unsigned int32 FramesTime;
  unsigned int32 FramesVideoDecompTime;
  unsigned int32 FramesAudioDecompTime;
  unsigned int32 FramesReadTime;
  unsigned int32 FramesIdleReadTime;
  unsigned int32 FramesThreadReadTime;
  unsigned int32 FramesBlitTime;
  unsigned int32 ReadBufferSize;
  unsigned int32 ReadBufferUsed;
  unsigned int32 FramesDataRate;
};

struct BINKHDR
{
  unsigned int32 Marker;
  unsigned int32 Size;
  unsigned int32 Frames;
  unsigned int32 LargestFrameSize;
  unsigned int32 InternalFrames;
  unsigned int32 Width;
  unsigned int32 Height;
  unsigned int32 FrameRate;
  unsigned int32 FrameRateDiv;
  unsigned int32 Flags;
  unsigned int32 NumTracks;
};

struct BINKTRACK
{
  unsigned int32 Frequency;
  unsigned int32 Bits;
  unsigned int32 Channels;
  unsigned int32 MaxSize;
  HBINK bink;
  unsigned int32 sndcomp;
  int32 trackindex;
};

BINKTRACK*;

struct BINKBUFFER
{
  unsigned int32 Width;
  unsigned int32 Height;
  unsigned int32 WindowWidth;
  unsigned int32 WindowHeight;
  unsigned int32 SurfaceType;
  void* Buffer;
  int32 BufferPitch;
  int32 ClientOffsetX;
  int32 ClientOffsetY;
  unsigned int32 ScreenWidth;
  unsigned int32 ScreenHeight;
  unsigned int32 ScreenDepth;
  unsigned int32 ExtraWindowWidth;
  unsigned int32 ExtraWindowHeight;
  unsigned int32 ScaleFlags;
  unsigned int32 StretchWidth;
  unsigned int32 StretchHeight;
  int32 surface;
  void* ddsurface;
  void* ddclipper;
  int32 destx;
  int32 desty;
  int32 wndx;
  int32 wndy;
  unsigned int32 wnd;
  int32 ddoverlay;
  int32 ddoffscreen;
  int32 lastovershow;
  int32 issoftcur;
  unsigned int32 cursorcount;
  void* buffertop;
  unsigned int32 type;
  int32 noclipping;
  int32 loadeddd;
  int32 loadedwin;
  void* dibh;
  void* dibbuffer;
  int32 dibpitch;
  void* dibinfo;
  unsigned int32 dibdc;
  unsigned int32 diboldbitmap;
};

BINKBUFFER*;

struct std::vector_type_event_record_ptr_
{
  int8 allocator;
  type_event_record** first;
  type_event_record** last;
  type_event_record** end;
};

struct type_record_shroud::type_shroud_change : type_point
{
  ushort old_value;
  ushort new_value;
};

struct std::vector_type_shroud_change_
{
  int8 allocator;
  type_record_shroud::type_shroud_change* first;
  type_record_shroud::type_shroud_change* last;
  type_record_shroud::type_shroud_change* end;
};

enum bool32
{
  false = 0x0,
  true = 0x1,
};

enum bool8
{
};

struct std::vector_mine_
{
  int8 allocator;
  mine* first;
  mine* last;
  mine* end;
};

struct std::vector_Sign_
{
  int8 allocator;
  Sign* first;
  Sign* last;
  Sign* end;
};

struct std::vector_town_
{
  int8 allocator;
  town* first;
  town* last;
  town* end;
};

struct _DDSCAPS2 { };

struct _DDSCAPS2
{
  DWORD dwCaps;
  DWORD dwCaps2;
  DWORD dwCaps3;
  _DDSCAPS2::$19AC68468C4510B3DC631A4E89752068 ;
};

union _DDSCAPS2::$19AC68468C4510B3DC631A4E89752068
{
  DWORD dwCaps4;
  DWORD dwVolumeDepth;
};

struct _DDBLTFX
{
  DWORD dwSize;
  DWORD dwDDFX;
  DWORD dwROP;
  DWORD dwDDROP;
  DWORD dwRotationAngle;
  DWORD dwZBufferOpCode;
  DWORD dwZBufferLow;
  DWORD dwZBufferHigh;
  DWORD dwZBufferBaseDest;
  DWORD dwZDestConstBitDepth;
  DWORD dwZDestConst;
  DWORD dwZSrcConstBitDepth;
  DWORD dwZSrcConst;
  DWORD dwAlphaEdgeBlendBitDepth;
  DWORD dwAlphaEdgeBlend;
  DWORD dwReserved;
  DWORD dwAlphaDestConstBitDepth;
  DWORD dwAlphaDestConst;
  DWORD dwAlphaSrcConstBitDepth;
  DWORD dwAlphaSrcConst;
  DWORD dwFillColor;
  DDCOLORKEY ddckDestColorkey;
  DDCOLORKEY ddckSrcColorkey;
};

struct combatManager::SElevationOverlay
{
  int16 terrainMask;
  int16 specialTerrainMask;
  int x;
  int y;
  int16[26] blockedSquares;
  int8* FileName;
};

struct combatManager::TWallTraits
{
  int16 x;
  int16 y;
  int16 hex;
  int8*[5] filenames;
  int8* name;
  int16 hitpoints;
};

struct CMCHideHero : CMapChange
{
  int m_heroId;
};

struct CSaveScreen : Bitmap16Bit
{
  bool screenSaved;
  int m_x;
  int m_y;
};

struct THeroClassTraits
{
  TTownType m_townType;
  int8* m_name;
  float m_aggression;
  int8[4] m_initialPrimarySkill;
  int8[4] m_gainPrimarySkillChance;
  int8[4] m_gainPrimarySkillChance10P;
  int8[28] m_gainSecondarySkillChance;
  int8[9] m_foundInTownType;
};

struct THeroTraits
{
  int m_sex;
  int m_race;
  THeroClass m_class;
  TSecondarySkill m_1stSkill;
  TSkillMastery m_1stSkillLevel;
  TSecondarySkill m_2ndSkill;
  TSkillMastery m_2ndSkillLevel;
  bool32 m_startsWithSpellbook;
  SpellID m_startingSpell;
  TCreatureType m_1stStack;
  TCreatureType m_2ndStack;
  TCreatureType m_3rdStack;
  int8* m_small_portrait_name;
  int8* m_large_portrait_name;
  bool m_allowedInRoE;
  bool m_allowedInABSoD;
  bool m_isCampaignHero;
  unsigned int attributes;
  int8* m_name;
  int m_1stStackLow;
  int m_1stStackHigh;
  int m_2ndStackLow;
  int m_2ndStackHigh;
  int m_3rdStackLow;
  int m_3rdStackHigh;
};

struct TRGBA
{
  uchar Red;
  uchar Green;
  uchar Blue;
  uchar Alpha;
};

struct RGB8
{
  uchar Red;
  uchar Green;
  uchar Blue;
};

struct std::vector_CObjectType_
{
  int8 allocator;
  CObjectType* first;
  CObjectType* last;
  CObjectType* end;
};

struct std::bitset_10_
{
  uint[1] bits;
};

struct type_belong_to_player_quest : type_quest
{
  int player;
};

struct type_quest
{
  type_quest::vftable_t* vftable;
  bool seer_hut;
  std::string proposal_text;
  std::string progress_text;
  std::string completion_text;
  int text_variant;
  int limit;
};

struct type_be_hero_quest : type_quest
{
  THeroID id;
};

struct type_resource_quest : type_quest
{
  int[7] resources;
};

struct type_creature_quest : type_quest
{
  std::vector amounts;
  std::vector types;
};

struct type_artifact_quest : type_quest
{
  std::vector artifacts;
};

struct type_monster_quest : type_quest
{
  uint map_id;
  type_point pos;
  TCreatureType type;
  int killer;
};

struct type_defeat_hero_quest : type_quest
{
  uint map_id;
  THeroID id;
  std::bitset_8_ completed;
};

struct type_skill_quest : type_quest
{
  uchar[4] skill;
};

struct type_experience_quest : type_quest
{
  int level;
};

struct type_quest::vftable_t
{
  void (__thiscall *)(type_quest* scalar_deleting_destructor, uchar ai_value);
  int (__thiscall *)(type_quest* scalar_deleting_destructor, int ai_value);
  bool (__thiscall *)(type_quest* scalar_deleting_destructor, hero* ai_value);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, hero* ai_value);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, hero* ai_value);
  void (__thiscall *)(type_quest* scalar_deleting_destructor);
  std::string* (__thiscall *)(type_quest* scalar_deleting_destructor, std::string* ai_value);
  std::string* (__thiscall *)(type_quest* scalar_deleting_destructor, std::string* ai_value);
  EQuestType (__thiscall *)(type_quest* scalar_deleting_destructor);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, THeroID ai_value, int can_complete);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, type_point ai_value, int can_complete);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, TAbstractFile* ai_value, int can_complete);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, TAbstractFile* ai_value);
  void (__thiscall *)(type_quest* scalar_deleting_destructor, TAbstractFile* ai_value);
  void (__thiscall *)(type_quest* scalar_deleting_destructor);
};

struct std::bitset_8_
{
  byte[4] bits;
};

struct combatManager::ObstacleInfo
{
  uint backgroundMask;
  uchar height;
  uchar width;
  uchar numSquares;
  uchar underlay;
  int8[8] sOffsets;
  int8* FileName;
};

struct std::bitset_12_
{
  byte[4] bits;
};

struct std::bitset_32_
{
  byte[4] bits;
};

struct type_artifact_offering : type_artifact
{
  TArtifactSlot source;
  int value;
};

struct TSSkillTraits
{
  int8* name;
  int8*[3] desc;
};

struct std::streambuf
{
  std::streambuf::vftable_t* vftable;
  int8* _Gbeg;
  int8* _Pbeg;
  int8** _IGbeg;
  int8** _IPbeg;
  int8* _Gnext;
  int8* _Pnext;
  int8** _IGnext;
  int8** _IPnext;
  int _Gcnt;
  int _Pcnt;
  int* _IGcnt;
  int* _IPcnt;
  void* _Loc;
};

struct std::wstring { };

struct SpriteDefHeader
{
  EResourceType type;
  int width;
  int height;
  int numseqs;
  unsigned int8[768] pal;
};

struct TGenericResource : resource
{
  void* Data;
  int Size;
};

struct CampaignIconPreview
{
  int index;
  int x;
  int y;
  int width;
  int height;
  int unknown1;
  int8* name;
  int widgetIndex;
  BinkManager::BinkManagerStruct bink;
};

struct GameSelectionHeadersStruct : NewSMapHeader
{
  SGameSetupOptions setup;
  int[8] unknown_1;
  int8[156] hero_status;
  int8[61] file_name;
  int8[301] map_desc;
  int8[2] unknown_2;
  FILETIME timestamp;
  SavedGameHeader header;
};

struct SavedGameHeader
{
  int8[8] id;
  int version;
  int game_version;
  NewSMapHeader map_header;
  SGameSetupOptions map_setup;
  bool campaign_game;
  byte[3] align;
  SCampaign campaign;
  std::string file_name;
  int16 difficultyRating;
  int numDeadPlayers;
  bool[8] dead_player;
  int[8] human_player;
  int current_player;
};

struct SpriteDataHeader
{
  int seqnumber;
  int numframes;
  int8* fname;
  int* frameOffsets;
};

struct CampaignRegionData
{
  int unknown;
  int8* background;
  int amount;
  CampaignRegionBaseData* regions;
};

struct CampaignRegionBaseData
{
  int unknown0;
  int x;
  int y;
  int8*[8][3] image;
};

struct std::pair_const_char_int_
{
  int8* first;
  int second;
};

struct std::vector_type_artifact_offering_
{
  int8 allocator;
  type_artifact_offering* first;
  type_artifact_offering* last;
  type_artifact_offering* end;
};

struct std::vector_vector_char_ptr__ptr__
{
  int8 allocator;
  std::vector_char_ptr_** first;
  std::vector_char_ptr_** last;
  std::vector_char_ptr_** end;
};

struct type_backpack_slot_widget : iconWidget { };

struct std::bitset_144_
{
  byte[20] bitset_array;
};

struct CombinationArtifact
{
  TArtifact type;
  std::bitset_144_ requirements;
};

struct TCampaignBrief::ScenarioStruct
{
  std::string name;
  uint offset;
  uint inflated_size;
  std::vector_bool_ prerequisites;
  std::string region_desc;
  uchar region_color;
  uchar difficulty;
  TCampaignBrief::MapTextStruct* prologue;
  TCampaignBrief::MapTextStruct* epilogue;
  bool retain_xp;
  bool retain_pskills;
  bool retain_sskills;
  bool retain_spellbook;
  bool retain_artifacts;
  int[8] placeholder_status;
  std::vector_int_ hero_placeholders;
  std::bitset_145_ crossover_creatures;
  std::bitset_144_ crossover_artifacts;
  t_scenario_start_options* options;
};

struct TSpellbookWindow::TSpellbookEntry
{
  SpellID Id;
  TSpellSchool School;
  TSkillMastery Mastery;
};

unsigned int8;

unsigned int;

unsigned int;

typedef Bytef Byte;

int8;

int;

typedef uIntf uInt;

typedef uLongf uLong;

void*;

void*;

voidpf (__cdecl *alloc_func)(voidpf opaque, uInt items, uInt size);

void (__cdecl *free_func)(voidpf opaque, voidpf address);

struct internal_state
{
  int dummy;
};

struct z_stream_s
{
  Bytef* next_in;
  uInt avail_in;
  uLong total_in;
  Bytef* next_out;
  uInt avail_out;
  uLong total_out;
  int8* msg;
  internal_state* state;
  alloc_func zalloc;
  free_func zfree;
  voidpf opaque;
  int data_type;
  uLong adler;
  uLong reserved;
};

struct z_stream_s { };

z_stream*;

typedef gzFile voidp;

struct TGzInflateBuf : std::streambuf
{
  std::streambuf* m_source;
  z_stream m_zstream;
  int8* m_input_buffer;
  int8* m_output_buffer;
  uint m_crc;
  bool m_is_compressed;
  bool m_end_of_file;
  bool m_stream_is_open;
  bool m_open;
};

struct type_text_scroller : widget
{
  int8* font_filename;
  std::vector_string_ text_lines;
  std::vector line_images;
  type_text_slider* text_slider;
  Bitmap16Bit* background;
};

struct type_text_slider : slider
{
  type_text_scroller* scroller;
};

struct CTextEntrySave : Bitmap16Bit
{
  bool saved;
};

struct combatManager::SCmbtHero
{
  int8* SpriteName;
  int castX;
  int castY;
  int castFrame;
};

struct CChatEdit : textEntryWidget { };

struct CAdventurMapChatEdit : CGameChatEdit { };

enum EMagicTerrain
{
  MAGIC_TERRAIN_INVALID = 0xFFFFFFFF,
  MAGIC_TERRAIN_COAST = 0x100000000,
  MAGIC_TERRAIN_MAGIC_PLAINS = 0x100000001,
  MAGIC_TERRAIN_CURSED_GROUND = 0x100000002,
  MAGIC_TERRAIN_HOLY_GROUND = 0x100000003,
  MAGIC_TERRAIN_EVIL_FOG = 0x100000004,
  MAGIC_TERRAIN_CLOVER_FIELD = 0x100000005,
  MAGIC_TERRAIN_LUCID_POOLS = 0x100000006,
  MAGIC_TERRAIN_FIERY_FIELDS = 0x100000007,
  MAGIC_TERRAIN_ROCKLANDS = 0x100000008,
  MAGIC_TERRAIN_MAGIC_CLOUDS = 0x100000009,
};

struct tilePoint
{
  int8 x;
  int8 y;
  byte[2] align;
};

struct ExtraObjectProperties
{
  bool impassable;
  bool omnidirectional;
  bool removable;
  int8* name;
  TAdventureObjectType type;
  bool decorative;
};

struct CImmDevice
{
  void* vftable;
  BOOL m_bInitialized;
  DWORD m_dwDeviceType;
  GUID m_guidDevice;
  BOOL m_bGuidValid;
  DWORD m_dwProductType;
};

CImmEffect*;

enum $8D5132959133546E347FCD1F4C29AA02
{
  IMMCACHE_NOT_ON_DEVICE = 0x0,
  IMMCACHE_ON_DEVICE = 0x1,
  IMMCACHE_SWAPPED_OUT = 0x2,
};

enum ECacheState
{
};

struct CEffectListElement
{
  CImmEffect* m_pImmEffect;
  CEffectListElement* m_pNext;
};

struct CEffectList
{
  CEffectListElement* m_pFirstEffect;
};

struct CImmEffectSuite
{
  BOOL m_bCurrentSuite;
  CEffectList m_EffectList;
};

FEELIT_CONSTANTFORCE*;

struct FEELIT_CONSTANTFORCE
{
  LONG lMagnitude;
};

struct FEELIT_RAMPFORCE
{
  LONG lStart;
  LONG lEnd;
};

struct FEELIT_PERIODIC
{
  DWORD dwMagnitude;
  LONG lOffset;
  DWORD dwPhase;
  DWORD dwPeriod;
};

struct FEELIT_CONDITION
{
  LONG lCenter;
  LONG lPositiveCoefficient;
  LONG lNegativeCoefficient;
  DWORD dwPositiveSaturation;
  DWORD dwNegativeSaturation;
  LONG lDeadBand;
};

struct FEELIT_TEXTURE
{
  DWORD dwSize;
  LONG lOffset;
  LONG lPosBumpMag;
  DWORD dwPosBumpWidth;
  DWORD dwPosBumpSpacing;
  LONG lNegBumpMag;
  DWORD dwNegBumpWidth;
  DWORD dwNegBumpSpacing;
};

struct FEELIT_CUSTOMFORCE
{
  DWORD cChannels;
  DWORD dwSamplePeriod;
  DWORD cSamples;
  LPLONG rglForceData;
};

struct FEELIT_ENVELOPE
{
  DWORD dwSize;
  DWORD dwAttackLevel;
  DWORD dwAttackTime;
  DWORD dwFadeLevel;
  DWORD dwFadeTime;
};

struct FEELIT_EFFECT
{
  DWORD dwSize;
  GUID guidEffect;
  DWORD dwFlags;
  DWORD dwDuration;
  DWORD dwSamplePeriod;
  DWORD dwGain;
  DWORD dwTriggerButton;
  DWORD dwTriggerRepeatInterval;
  DWORD cAxes;
  DWORD* rgdwAxes;
  LONG* rglDirection;
  FEELIT_ENVELOPE* lpEnvelope;
  DWORD cbTypeSpecificParams;
  LPVOID lpvTypeSpecificParams;
  DWORD dwStartDelay;
};

struct FEELIT_ENCLOSURE
{
  DWORD dwSize;
  RECT rectBoundary;
  DWORD dwTopAndBottomWallThickness;
  DWORD dwLeftAndRightWallThickness;
  LONG lTopAndBottomWallStiffness;
  LONG lLeftAndRightWallStiffness;
  DWORD dwStiffnessMask;
  DWORD dwClippingMask;
  DWORD dwTopAndBottomWallSaturation;
  DWORD dwLeftAndRightWallSaturation;
  void* piInsideEffect;
};

struct std::vector_int_
{
  int8 allocator;
  int* first;
  int* last;
  int* end;
};

struct std::vector_uint_
{
  int8 allocator;
  uint* first;
  uint* last;
  uint* end;
};

struct CDPlayConnection
{
  _GUID guidSP;
  uchar* pConnection;
  int8[128] sName;
  uint size;
};

struct IDirectPlay3 : IDirectPlay2 { };

struct CDPlay::vftable_t
{
  void (__thiscall *)(CDPlay* scalar_deleting_destructor, uchar this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, CDPlayConnection* this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, int8* this, ulong flag, ulong Init, int8* this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, _GUID* this, int8* flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor);
  ulong (__thiscall *)(CDPlay* scalar_deleting_destructor, int8* this, void* flag, ulong Init, void* this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this);
  ulong (__thiscall *)(CDPlay* scalar_deleting_destructor, int8* this, void* flag, ulong Init, bool this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, int8* flag, int8* Init, ulong this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, void* flag, ulong Init, ulong this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, int8* flag, int8* Init, ulong this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, void* flag, ulong Init, ulong this);
  void* (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong* flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, int8* flag, int Init, int8* this, int InitConnection);
  void* (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong* flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, int8* flag, int Init, int8* this, int InitConnection);
  ulong (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, int8* flag, void* Init, ulong this, bool InitConnection);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPSESSIONDESC2* this);
  DPSESSIONDESC2* (__thiscall *)(CDPlay* scalar_deleting_destructor);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, CAutoArray_CDPlayConnection_* this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, CAutoArray_CDPlaySession_* this, ulong flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, CAutoArray_CDPlayGroup_* this, _GUID* flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, CAutoArray_CDPlayPlayer_* this, _GUID* flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, CAutoArray_CDPlayPlayer_* this, ulong flag, _GUID* Init, ulong this);
  void (__thiscall *)(CDPlay* scalar_deleting_destructor, _GUID this);
  _GUID* (__thiscall *)(CDPlay* scalar_deleting_destructor);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, void* this, ulong flag, ulong Init, ulong this, bool InitConnection);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, int8* this, ulong flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong* this, ulong* flag, CDPlayMsg* Init, ulong this);
  void (__thiscall *)(CDPlay* scalar_deleting_destructor, int this, int8* flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor);
  uchar* (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong* flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPCAPS* this, bool flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong flag, ulong* Init, ulong* this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong flag, ulong* Init, ulong* this);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, ulong flag, CDPlayMsg* Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, CDPlayMsg* flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_ADDGROUPTOGROUP* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_ADDPLAYERTOGROUP* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_CHAT* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_ADDGROUPTOGROUP* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_ADDPLAYERTOGROUP* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_SECUREMESSAGE* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_GENERIC* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_SETPLAYERORGROUPDATA* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_SETPLAYERORGROUPNAME* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_SETSESSIONDESC* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_STARTSESSION* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_GENERIC* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_CREATEPLAYERORGROUP* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPMSG_DESTROYPLAYERORGROUP* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, DPNAME* flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, ulong this, DPNAME* flag, ulong Init);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, DPSESSIONDESC2* this, ulong flag);
  bool (__thiscall *)(CDPlay* scalar_deleting_destructor, _GUID* this, void* flag, ulong Init, DPNAME* this, ulong InitConnection);
};

struct DPCOMPORTADDRESS
{
  DWORD dwComPort;
  DWORD dwBaudRate;
  DWORD dwStopBits;
  DWORD dwParity;
  DWORD dwFlowControl;
};

enum heroWindow::__unnamed
{
  MIN_WIDGET_ID = 0xFFFF0001,
  MAX_WIDGET_ID = 0x10000FFFF,
  DropShadowXOffset = 0x200000008,
  DropShadowYOffset = 0x200000008,
};

struct type_icon_definition
{
  int x;
  int y;
  int width;
  int height;
  int8* name;
};

struct type_doll_slot_definition : type_icon_definition
{
  TArtifactSlot slot;
};

struct bitmapBackedTextWidget : textWidget
{
  Bitmap816* Background;
};

struct HeroPlaceholder
{
  CObject* object;
  int8 player;
  THeroID id;
  int8 power;
};

struct TPoint
{
  int x;
  int y;
};

struct TObjectType::_TImageInfo
{
  TPoint objSize;
  std::bitset_48_ drawMask;
  std::bitset_48_ shadowMask;
};

struct TObjectType
{
  int _imageNum;
  std::bitset_48_ _passableMask;
  std::bitset_48_ _triggerMask;
  std::bitset_10_ _terrainMask;
  std::bitset_10_ _terrainRecommendedMask;
  TAdventureObjectType _type;
  int _subtype;
  int _slotCategory;
  bool _isUnderlay;
  bool _hasTrigger;
  TPoint _triggerCell;
  TObjectType::_TImageInfo _imageInfo;
};

struct std::vector_NewmapCell__TObjectCell_
{
  int8 allocator;
  NewmapCell::TObjectCell* first;
  NewmapCell::TObjectCell* last;
  NewmapCell::TObjectCell* end;
};

struct TAbstractFile
{
  TAbstractFile::vftable_t* vftable;
};

struct TStreamBufFile : TAbstractFile
{
  TGzInflateBuf* stream;
};

struct ResourceManager::TCacheMapKey
{
  int8[12] name;
  bool b;
};

struct std::map_ResourceManager::TCacheMapKey_resource_ptr__
{
  int8 allocator;
  int8 key_compare;
  std::map_ResourceManager::TCacheMapKey_resource_ptr__::_Node* _Head;
  bool _Multi;
  size_t _Size;
};

struct HeroIdentity
{
  int8 portrait;
  std::string name;
};

struct HeroPlayerInfo : HeroIdentity
{
  std::bitset_8_ players;
};

struct std::pair_int_HeroPlayerInfo_
{
  int first;
  HeroPlayerInfo second;
};

struct std::pair_ResourceManager::TCacheMapKey_resource_ptr__
{
  ResourceManager::TCacheMapKey first;
  resource* second;
};

struct std::map_ResourceManager::TCacheMapKey_resource_ptr__::_Node
{
  std::map_ResourceManager::TCacheMapKey_resource_ptr__::_Node* _Left;
  std::map_ResourceManager::TCacheMapKey_resource_ptr__::_Node* _Parent;
  std::map_ResourceManager::TCacheMapKey_resource_ptr__::_Node* _Right;
  std::pair_ResourceManager::TCacheMapKey_resource_ptr__ _Value;
  int _Color;
};

struct std::map_int_HeroPlayerInfo_::_Node
{
  _Node* _Left;
  _Node* _Parent;
  _Node* _Right;
  std::pair_int_HeroPlayerInfo_ _Value;
  int _Color;
};

struct t_lod_file_adapter : TAbstractFile
{
  LODFile* lod_file;
};

struct std::pair_const_char_ptr_LODFile_
{
  int8* first;
  LODFile second;
};

struct TSeerReward
{
  TSeerRewardType rewardType;
  TSeerReward::SeerRewardUnion reward;
};

struct TGarrisonWindow : type_garrison_base_window { };

enum EQuestType
{
  QUEST_NONE = 0x0,
  QUEST_EXPERIENCE = 0x1,
  QUEST_SKILL = 0x2,
  QUEST_DEFEAT_HERO = 0x3,
  QUEST_DEFEAT_MONSTER = 0x4,
  QUEST_ARTIFACT = 0x5,
  QUEST_CREATURE = 0x6,
  QUEST_RESOURCE = 0x7,
  QUEST_BE_HERO = 0x8,
  QUEST_BELONG_TO_PLAYER = 0x9,
};

enum FSpellEffectFlags
{
  SPELL_EFFECT_TRANSPARENT = 0x100,
  SPELL_EFFECT_BELOW = 0x100000000,
  SPELL_EFFECT_MIDDLE = 0x100000001,
  SPELL_EFFECT_ABOVE = 0x100000002,
  SPELL_EFFECT_AHEAD = 0x100000003,
  SPELL_EFFECT_LEFT_BELOW = 0x100000004,
  SPELL_EFFECT_UNSPECIFIED = 0x10000000F,
};

struct type_dialog_resource
{
  EGameResource resource;
  int qualifier;
};

struct std::vector_type_dialog_resource_
{
  int8 allocator;
  type_dialog_resource* first;
  type_dialog_resource* last;
  type_dialog_resource* end;
};

struct t_complex_net_message
{
  t_complex_net_message::vftable_t* vftable;
  CNetMsg netmsg;
};

struct CAutoArray_CDPlayPlayer_
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct CAutoArray_int_
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct CAutoArray_CDPlayConnection_
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct CAutoArray_CDPlaySession_
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct CAutoArray_CDPlayGroup_
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct CAutoArray_CDPlayAddressElement_
{
  CAutoArray::vftable_t* vftable;
  DWORD step;
  DWORD* pArray;
  DWORD allocSize;
  DWORD size;
};

struct DPCHAT
{
  DWORD dwSize;
  DWORD dwFlags;
  int8* lpszMessageA;
};

typedef DPID DWORD;

struct DPMSG_CHAT
{
  DWORD dwType;
  DWORD dwFlags;
  DPID idFromPlayer;
  DPID idToPlayer;
  DPID idToGroup;
  DPCHAT* lpChat;
};

struct DPMSG_ADDGROUPTOGROUP
{
  DWORD dwType;
  DPID dpIdParentGroup;
  DPID dpIdGroup;
};

struct DPMSG_ADDPLAYERTOGROUP
{
  DWORD dwType;
  DPID dpIdGroup;
  DPID dpIdPlayer;
};

struct DPMSG_SECUREMESSAGE
{
  DWORD dwType;
  DWORD dwFlags;
  DPID dpIdFrom;
  LPVOID lpData;
  DWORD dwDataSize;
};

enum DirectPlayMessages
{
  DPSYS_CREATEPLAYERORGROUP = 0x3,
  DPSYS_DESTROYPLAYERORGROUP = 0x5,
  DPSYS_ADDPLAYERTOGROUP = 0x7,
  DPSYS_DELETEPLAYERFROMGROUP = 0x21,
  DPSYS_SESSIONLOST = 0x31,
  DPSYS_HOST = 0x101,
  DPSYS_SETPLAYERORGROUPDATA = 0x102,
  DPSYS_SETPLAYERORGROUPNAME = 0x103,
  DPSYS_SETSESSIONDESC = 0x104,
  DPSYS_ADDGROUPTOGROUP = 0x105,
  DPSYS_DELETEGROUPFROMGROUP = 0x106,
  DPSYS_SECUREMESSAGE = 0x107,
  DPSYS_STARTSESSION = 0x108,
  DPSYS_CHAT = 0x109,
  DPSYS_SETGROUPOWNER = 0x10A,
  DPSYS_SENDCOMPLETE = 0x10D,
};

struct DPMSG_GENERIC
{
  DirectPlayMessages dwType;
};

struct DPMSG_SETPLAYERORGROUPDATA
{
  DWORD dwType;
  DWORD dwPlayerType;
  DPID dpId;
  LPVOID lpData;
  DWORD dwDataSize;
};

struct DPMSG_SETSESSIONDESC
{
  DWORD dwType;
  DPSESSIONDESC2 dpDesc;
};

struct DPMSG_STARTSESSION
{
  DWORD dwType;
  DPLCONNECTION* lpConn;
};

enum PlayerTypeValue
{
  DPPLAYERTYPE_GROUP = 0x0,
  DPPLAYERTYPE_PLAYER = 0x1,
};

struct DPMSG_CREATEPLAYERORGROUP
{
  DWORD dwType;
  PlayerTypeValue dwPlayerType;
  DPID dpId;
  DWORD dwCurrentPlayers;
  LPVOID lpData;
  DWORD dwDataSize;
  DPNAME dpnName;
  DPID dpIdParent;
  DWORD dwFlags;
};

struct DPMSG_DESTROYPLAYERORGROUP
{
  DWORD dwType;
  DWORD dwPlayerType;
  DPID dpId;
  LPVOID lpLocalData;
  DWORD dwLocalDataSize;
  LPVOID lpRemoteData;
  DWORD dwRemoteDataSize;
  DPNAME dpnName;
  DPID dpIdParent;
  DWORD dwFlags;
};

struct DPMSG_SETPLAYERORGROUPNAME
{
  DWORD dwType;
  DWORD dwPlayerType;
  DPID dpId;
  DPNAME dpnName;
};

struct CDPlayLobby::vftable_t : CDPlay::vftable_t
{
  bool (__thiscall *)(CDPlay* RegisterApp, int8* this, int8* EnumLobbyConnections, int8* this, _GUID SetGroupConnectionSettings);
  bool (__thiscall *)(CDPlay* RegisterApp, CAutoArray_CDPlayConnection_* this);
  bool (__thiscall *)(CDPlay* RegisterApp, ulong this, DPLCONNECTION* EnumLobbyConnections);
  DPLCONNECTION* (__thiscall *)(CDPlay* RegisterApp, ulong this);
  bool (__thiscall *)(CDPlay* RegisterApp, CAutoArray_CDPlayGroup_* this, ulong EnumLobbyConnections, ulong this);
  bool (__thiscall *)(CDPlay* RegisterApp, CAutoArray_CDPlayPlayer_* this, ulong EnumLobbyConnections, ulong this);
  bool (__thiscall *)(CDPlay* RegisterApp, CAutoArray_CDPlayPlayer_* this, ulong EnumLobbyConnections, _GUID* this, ulong SetGroupConnectionSettings);
  bool (__thiscall *)(CDPlay* RegisterApp, void* this, ulong EnumLobbyConnections, CAutoArray_CDPlayAddressElement_* this);
  bool (__thiscall *)(CDPlay* RegisterApp, ulong this, int8* EnumLobbyConnections);
  bool (__thiscall *)(CDPlay* RegisterApp, ulong this, CDPlayMsg* EnumLobbyConnections);
  bool (__thiscall *)(CDPlay* RegisterApp, _GUID* this, ulong EnumLobbyConnections, void* this);
};

struct CPlayerDropMsg : CNetMsg
{
  uint m_dpid;
};

struct IUnknown_vtbl
{
  HRESULT (__stdcall *)(IUnknown* QueryInterface, _GUID* this, void** AddRef);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
};

struct IUnknown
{
  IDirectPlay4_vtbl* vftable;
};

struct IDirectPlay2 : IUnknown { };

struct IDirectPlay2_vtbl
{
  HRESULT (__stdcall *)(IUnknown* QueryInterface, _GUID* this, void** AddRef);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int* this, DPNAME* AddRef, void* this, unsigned int Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int* this, DPNAME* AddRef, void* this, void* Release, unsigned int this, unsigned int AddPlayerToGroup);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, _GUID* AddRef, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, _GUID* this, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, _GUID* this, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef, int (__stdcall *)(DPSESSIONDESC2* QueryInterface, unsigned int* this, unsigned int AddRef, void* this) this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPCAPS* this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int* AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, DPCAPS* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, void* this, unsigned int* AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, _GUID* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int* this, unsigned int* AddRef, unsigned int this, void* Release, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int AddRef, unsigned int this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef);
};

struct DPSECURITYDESC
{
  DWORD dwSize;
  DWORD dwFlags;
  LPSTR lpszSSPIProviderA;
  LPSTR lpszCAPIProviderA;
  DWORD dwCAPIProviderType;
  DWORD dwEncryptionAlgorithm;
};

struct DPCREDENTIALS
{
  DWORD dwSize;
  DWORD dwFlags;
  LPSTR lpszUsernameA;
  LPSTR lpszPasswordA;
  LPSTR lpszDomainA;
};

struct IDirectPlay3_vtbl
{
  HRESULT (__stdcall *)(IUnknown* QueryInterface, _GUID* this, void** AddRef);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int* this, DPNAME* AddRef, void* this, unsigned int Release, DPSESSION_Flags this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int* this, DPNAME* AddRef, void* this, void* Release, unsigned int this, unsigned int AddPlayerToGroup);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, _GUID* AddRef, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, _GUID* this, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, _GUID* this, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef, int (__stdcall *)(DPSESSIONDESC2* QueryInterface, unsigned int* this, unsigned int AddRef, void* this) this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPCAPS* this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int* AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, DPCAPS* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, void* this, unsigned int* AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, _GUID* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int* this, unsigned int* AddRef, unsigned int this, void* Release, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, unsigned int AddRef, unsigned int this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, void* AddRef, unsigned int this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay2* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int* AddRef, DPNAME* this, void* Release, unsigned int this, unsigned int AddPlayerToGroup);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, _GUID* this, int (__stdcall *)(_GUID* QueryInterface, void* this, unsigned int AddRef, DPNAME* this, unsigned int Release, void* this) AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, _GUID* AddRef, int (__stdcall *)(unsigned int QueryInterface, unsigned int this, DPNAME* AddRef, unsigned int this, void* Release) this, void* Release, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef, void* this, unsigned int* Release);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, void* this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, DPSESSIONDESC2* this, unsigned int AddRef, DPSECURITYDESC* this, DPCREDENTIALS* Release);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef, unsigned int this, DPCHAT* Release);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef, DPLCONNECTION* this);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int* AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int* AddRef);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int AddRef, void* this, unsigned int* Release);
  HRESULT (__stdcall *)(IDirectPlay3* QueryInterface, unsigned int this, unsigned int* AddRef);
};

struct IDirectPlayLobby
{
  IDirectPlayLobby3_vtbl* vftable;
};

struct DPLAPPINFO
{
  DWORD dwSize;
  GUID guidApplication;
  LPSTR lpszAppNameA;
};

struct IDirectPlayLobby_vtbl
{
  HRESULT (__stdcall *)(IUnknown* QueryInterface, _GUID* this, void** AddRef);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, IDirectPlay2** AddRef, IUnknown* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, _GUID* this, _GUID* AddRef, void* this, unsigned int Release, void* this, unsigned int* Connect);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, int (__stdcall *)(_GUID* QueryInterface, unsigned int this, void* AddRef, void* this) this, void* AddRef, unsigned int this, void* Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, int (__stdcall *)(_GUID* QueryInterface, void* this, unsigned int AddRef) this, _GUID* AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, int (__stdcall *)(DPLAPPINFO* QueryInterface, void* this, unsigned int AddRef) this, void* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, unsigned int* this, void* Release, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int* AddRef, DPLCONNECTION* this, void* Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, DPLCONNECTION* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, void* this);
};

struct DPCOMPOUNDADDRESSELEMENT
{
  GUID guidDataType;
  DWORD dwDataSize;
  LPVOID lpData;
};

struct IDirectPlayLobby2_vtbl
{
  HRESULT (__stdcall *)(IUnknown* QueryInterface, _GUID* this, void** AddRef);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  unsigned int (__stdcall *)(IUnknown* QueryInterface);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, IDirectPlay2** AddRef, IUnknown* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, _GUID* this, _GUID* AddRef, void* this, unsigned int Release, void* this, unsigned int* Connect);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, int (__stdcall *)(_GUID* QueryInterface, unsigned int this, void* AddRef, void* this) this, void* AddRef, unsigned int this, void* Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, int (__stdcall *)(_GUID* QueryInterface, void* this, unsigned int AddRef) this, _GUID* AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, int (__stdcall *)(DPLAPPINFO* QueryInterface, void* this, unsigned int AddRef) this, void* AddRef, unsigned int this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, void* AddRef, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, unsigned int* this, void* Release, unsigned int* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int* AddRef, DPLCONNECTION* this, void* Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, void* this, unsigned int Release);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, DPLCONNECTION* this);
  HRESULT (__stdcall *)(IDirectPlayLobby* QueryInterface, unsigned int this, unsigned int AddRef, void* this);
  HRESULT (__stdcall *)(IDirectPlayLobby2* QueryInterface, DPCOMPOUNDADDRESSELEMENT* this, unsigned int AddRef, void* this, unsigned int* Release);
};

enum DPSESSION_Flags
{
  DPSESSION_NEWPLAYERSDISABLED = 0x1,
  DPSESSION_MIGRATEHOST = 0x4,
  DPSESSION_NOMESSAGEID = 0x8,
  DPSESSION_JOINDISABLED = 0x20,
  DPSESSION_KEEPALIVE = 0x40,
  DPSESSION_NODATAMESSAGES = 0x80,
  DPSESSION_SECURESERVER = 0x100,
  DPSESSION_PRIVATE = 0x200,
  DPSESSION_PASSWORDREQUIRED = 0x400,
  DPSESSION_MULTICASTSERVER = 0x800,
  DPSESSION_CLIENTSERVER = 0x1000,
  DPSESSION_DIRECTPLAYPROTOCOL = 0x2000,
  DPSESSION_NOPRESERVEORDER = 0x4000,
};

enum DPlay_Error
{
  DP_OK = 0x0,
  DPERR_ALREADYINITIALIZED = 0x88770005,
  DPERR_ACCESSDENIED = 0x8877000A,
  DPERR_ACTIVEPLAYERS = 0x88770014,
  DPERR_BUFFERTOOSMALL = 0x8877001E,
  DPERR_CANTADDPLAYER = 0x88770028,
  DPERR_CANTCREATEGROUP = 0x88770032,
  DPERR_CANTCREATEPLAYER = 0x8877003C,
  DPERR_CANTCREATESESSION = 0x88770046,
  DPERR_CAPSNOTAVAILABLEYET = 0x88770050,
  DPERR_EXCEPTION = 0x8877005A,
  DPERR_GENERIC = 0x180004005,
  DPERR_INVALIDFLAGS = 0x188770078,
  DPERR_INVALIDOBJECT = 0x188770082,
  DPERR_INVALIDPARAM = 0x280070057,
  DPERR_INVALIDPARAMS = 0x280070057,
  DPERR_INVALIDPLAYER = 0x288770096,
  DPERR_INVALIDGROUP = 0x28877009B,
  DPERR_NOCAPS = 0x2887700A0,
  DPERR_NOCONNECTION = 0x2887700AA,
  DPERR_NOMEMORY = 0x38007000E,
  DPERR_OUTOFMEMORY = 0x38007000E,
  DPERR_NOMESSAGES = 0x3887700BE,
  DPERR_NONAMESERVERFOUND = 0x3887700C8,
  DPERR_NOPLAYERS = 0x3887700D2,
  DPERR_NOSESSIONS = 0x3887700DC,
  DPERR_PENDING = 0x48000000A,
  DPERR_SENDTOOBIG = 0x4887700E6,
  DPERR_TIMEOUT = 0x4887700F0,
  DPERR_UNAVAILABLE = 0x4887700FA,
  DPERR_UNSUPPORTED = 0x580004001,
  DPERR_BUSY = 0x58877010E,
  DPERR_USERCANCEL = 0x588770118,
  DPERR_NOINTERFACE = 0x680004002,
  DPERR_CANNOTCREATESERVER = 0x688770122,
  DPERR_PLAYERLOST = 0x68877012C,
  DPERR_SESSIONLOST = 0x688770136,
  DPERR_UNINITIALIZED = 0x688770140,
  DPERR_NONEWPLAYERS = 0x78877013A,
  DPERR_INVALIDPASSWORD = 0x788770154,
  DPERR_CONNECTING = 0x78877015E,
  DPERR_CONNECTIONLOST = 0x788770168,
  DPERR_UNKNOWNMESSAGE = 0x788770172,
  DPERR_CANCELFAILED = 0x78877017C,
  DPERR_INVALIDPRIORITY = 0x788770186,
  DPERR_NOTHANDLED = 0x788770190,
  DPERR_CANCELLED = 0x78877019A,
  DPERR_ABORTED = 0x7887701A4,
  DPERR_BUFFERTOOLARGE = 0x7887703E8,
  DPERR_CANTCREATEPROCESS = 0x7887703F2,
  DPERR_APPNOTSTARTED = 0x7887703FC,
  DPERR_INVALIDINTERFACE = 0x788770406,
  DPERR_NOSERVICEPROVIDER = 0x788770410,
  DPERR_UNKNOWNAPPLICATION = 0x78877041A,
  DPERR_NOTLOBBIED = 0x78877042E,
  DPERR_SERVICEPROVIDERLOADED = 0x788770438,
  DPERR_ALREADYREGISTERED = 0x788770442,
  DPERR_NOTREGISTERED = 0x78877044C,
  DPERR_AUTHENTICATIONFAILED = 0x7887707D0,
  DPERR_CANTLOADSSPI = 0x7887707DA,
  DPERR_ENCRYPTIONFAILED = 0x7887707E4,
  DPERR_SIGNFAILED = 0x7887707EE,
  DPERR_CANTLOADSECURITYPACKAGE = 0x7887707F8,
  DPERR_ENCRYPTIONNOTSUPPORTED = 0x788770802,
  DPERR_CANTLOADCAPI = 0x78877080C,
  DPERR_NOTLOGGEDIN = 0x788770816,
  DPERR_LOGONDENIED = 0x788770820,
};

struct IDirectPlay4_vtbl : IDirectPlay3_vtbl
{
  HRESULT (__stdcall *)(IDirectPlay4* GetGroupOwner, DPID this, LPDPID SetGroupOwner);
  HRESULT (__stdcall *)(IDirectPlay4* GetGroupOwner, DPID this, DPID SetGroupOwner);
  HRESULT (__stdcall *)(IDirectPlay4* GetGroupOwner, DPID this, DPID SetGroupOwner, DWORD this, LPVOID SendEx, DWORD this, DWORD GetMessageQueue, DWORD this, LPVOID CancelMessage, LPDWORD this);
  HRESULT (__stdcall *)(IDirectPlay4* GetGroupOwner, DPID this, DPID SetGroupOwner, DWORD this, LPDWORD SendEx, LPDWORD this);
  HRESULT (__stdcall *)(IDirectPlay4* GetGroupOwner, DWORD this, DWORD SetGroupOwner);
  HRESULT (__stdcall *)(IDirectPlay4* GetGroupOwner, DWORD this, DWORD SetGroupOwner, DWORD this);
};

struct IDirectPlay4 : IDirectPlay3 { };

DPID*;

struct CSessionLostMsg : CNetMsg { };

struct CSetAsHostMsg : CNetMsg { };

struct CDPlayPlayer
{
  int8[256] m_sName;
  ulong m_dpid;
};

struct CDPlayGroup : CDPlayPlayer { };

struct CDPlaySession
{
  uint dwFlags;
  _GUID guidInstance;
  _GUID guidApp;
  uint maxPlayers;
  uint playerCount;
  int8[128] sessionName;
  int8[80] password;
  uint dwUser1;
  uint dwUser2;
  uint dwUser3;
  uint dwUser4;
};

struct DPAPPLICATIONDESC
{
  DWORD dwSize;
  DWORD dwFlags;
  LPSTR lpszApplicationNameA;
  GUID guidApplication;
  LPSTR lpszFilenameA;
  LPSTR lpszCommandLineA;
  LPSTR lpszPathA;
  LPSTR lpszCurrentDirectoryA;
  LPSTR lpszDescriptionA;
};

struct IDirectPlayLobby3 : IDirectPlayLobby2 { };

struct IDirectPlayLobby3_vtbl : IDirectPlayLobby2_vtbl
{
  HRESULT (__stdcall *)(IDirectPlayLobby3* ConnectEx, DWORD this, IID* RegisterApplication, LPVOID* this, IUnknown* UnregisterApplication);
  HRESULT (__stdcall *)(IDirectPlayLobby3* ConnectEx, DWORD this, DPAPPLICATIONDESC* RegisterApplication);
  HRESULT (__stdcall *)(IDirectPlayLobby3* ConnectEx, DWORD this, _GUID* RegisterApplication);
  HRESULT (__stdcall *)(IDirectPlayLobby3* ConnectEx, DWORD this);
};

struct CDPlayAddressElement
{
  _GUID m_guid;
  uchar* m_pData;
  uint m_size;
};

enum DPLMSG_SYSTEMMESSAGE_Type
{
  DPLSYS_CONNECTIONSETTINGSREAD = 0x1,
  DPLSYS_DPLAYCONNECTFAILED = 0x2,
  DPLSYS_DPLAYCONNECTSUCCEEDED = 0x3,
  DPLSYS_APPTERMINATED = 0x4,
  DPLSYS_SETPROPERTY = 0x5,
  DPLSYS_SETPROPERTYRESPONSE = 0x6,
  DPLSYS_GETPROPERTY = 0x7,
  DPLSYS_GETPROPERTYRESPONSE = 0x8,
  DPLSYS_NEWSESSIONHOST = 0x9,
  DPLSYS_NEWCONNECTIONSETTINGS = 0xA,
};

struct DPLMSG_SYSTEMMESSAGE
{
  DPLMSG_SYSTEMMESSAGE_Type dwType;
  GUID guidInstance;
};

struct CPingResponseMsg : CNetMsg
{
  uint m_pingTime;
};

struct std::bitset_9_
{
  uint bits;
};

struct TRandomDwelling
{
  int townId;
  ushort towns;
  uchar playerOwner;
  uchar minLVL;
  uchar maxLVL;
  byte[3] gap_9;
  CObject* object;
};

struct type_artifact_effect
{
  type_artifact_effect::vftable_t* vftable;
};

struct type_scouting_artifact : type_artifact_effect
{
  int bonus;
};

struct type_combat_artifact : type_artifact_effect
{
  int bonus;
};

struct type_movement_artifact : type_combat_artifact { };

struct type_spellcaster_artifact : type_combat_artifact { };

struct type_tome_artifact : type_combat_artifact
{
  TSpellSchool school;
};

struct type_antimagic_artifact : type_artifact_effect
{
  int max_level;
};

struct type_antimorale_artifact : type_artifact_effect { };

struct type_antiluck_artifact : type_artifact_effect { };

struct type_income_artifact : type_artifact_effect
{
  int amount;
  EGameResource resource;
};

struct type_creature_growth_artifact : type_artifact_effect
{
  int level;
  int bonus;
};

struct type_spell_artifact : type_artifact_effect
{
  SpellID spell;
};

struct type_shooter_bonus_artifact : type_combat_artifact { };

struct type_statue_of_legion_artifact : type_artifact_effect { };

struct type_elixir_of_life_artifact : type_artifact_effect { };

struct type_might_artifact : type_combat_artifact { };

struct type_power_artifact : type_combat_artifact { };

struct type_knowledge_artifact : type_combat_artifact { };

struct type_morale_artifact : type_combat_artifact { };

struct type_luck_artifact : type_combat_artifact { };

struct type_base_necromancy_artifact : type_combat_artifact { };

struct type_necromancy_artifact : type_base_necromancy_artifact { };

struct type_duration_artifact : type_power_artifact { };

struct type_school_artifact : type_power_artifact
{
  TSpellSchool school;
};

struct type_angelic_alliance_artifact : type_might_artifact { };

struct type_undead_king_cloak_artifact : type_base_necromancy_artifact { };

struct std::vector_type_artifact_effect_ptr_
{
  int8 allocator;
  type_artifact_effect** first;
  type_artifact_effect** last;
  type_artifact_effect** end;
};

enum CombinationArtifactType
{
  COMBO_NONE = 0xFFFFFFFF,
  COMBO_ANGELIC_ALLIANCE = 0x100000000,
  COMBO_CLOAK_OF_THE_UNDEAD_KING = 0x100000001,
  COMBO_ELIXIR_OF_LIFE = 0x100000002,
  COMBO_ARMOR_OF_THE_DAMNED = 0x100000003,
  COMBO_STATUE_OF_LEGION = 0x100000004,
  COMBO_POWER_OF_THE_DRAGON_FATHER = 0x100000005,
  COMBO_TITANS_THUNDER = 0x100000006,
  COMBO_ADMIRALS_HAT = 0x100000007,
  COMBO_BOW_OF_THE_SHARPSHOOTER = 0x100000008,
  COMBO_WIZARDS_WELL = 0x100000009,
  COMBO_RING_OF_THE_MAGI = 0x10000000A,
  COMBO_CORNUCOPIA = 0x10000000B,
};

enum DialogReturnType
{
  DIALOG_RETURN_NONE = 0xFFFFFFFF,
  DIALOG_RETURN_CANCEL = 0x100007801,
  DIALOG_RETURN_OK = 0x100007802,
  DIALOG_RETURN_ACCEPT = 0x100007805,
  DIALOG_RETURN_DECLINE = 0x100007806,
  DIALOG_SELECT_LEFT = 0x100007809,
  DIALOG_SELECT_RIGHT = 0x10000780A,
};

struct AnimHeaderStruct
{
  int8[40] filename;
  int offset;
};

struct IDirectDraw4
{
  IDirectDraw4Vtbl* lpVtbl;
};

struct IDirectDraw4Vtbl
{
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, IID* This, LPVOID* riid);
  ULONG (__stdcall *)(IDirectDraw4* QueryInterface);
  ULONG (__stdcall *)(IDirectDraw4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, DWORD This, LPDIRECTDRAWCLIPPER riid, IUnknown* ppvObj);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, DWORD This, LPPALETTEENTRY riid, LPDIRECTDRAWPALETTE* ppvObj, IUnknown* AddRef);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDDSURFACEDESC This, LPDIRECTDRAWSURFACE4 riid, IUnknown* ppvObj);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDIRECTDRAWSURFACE4 This, LPDIRECTDRAWSURFACE4* riid);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, DWORD This, LPDDSURFACEDESC riid, LPVOID ppvObj, LPDDENUMMODESCALLBACK AddRef);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, DWORD This, LPDDSURFACEDESC riid, LPVOID ppvObj, LPDDENUMSURFACESCALLBACK AddRef);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDDCAPS This, LPDDCAPS riid);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDDSURFACEDESC This);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDWORD This, LPDWORD riid);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDIRECTDRAWSURFACE4* This);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDWORD This);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDWORD This);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPBOOL This);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, GUID* This);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, HWND This, DWORD riid);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, DWORD This, DWORD riid, DWORD ppvObj);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, DWORD This, HANDLE riid);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDDSCAPS2 This, LPDWORD riid, LPDWORD ppvObj);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, HDC This, LPDIRECTDRAWSURFACE4* riid);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface);
  HRESULT (__stdcall *)(IDirectDraw4* QueryInterface, LPDDDEVICEIDENTIFIER This, DWORD riid);
};

int;

IDirectDrawClipper*;

struct IDirectDrawClipper
{
  IDirectDrawClipperVtbl* lpVtbl;
};

struct IDirectDrawClipperVtbl
{
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, IID* This, LPVOID* riid);
  ULONG (__stdcall *)(IDirectDrawClipper* QueryInterface);
  ULONG (__stdcall *)(IDirectDrawClipper* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, LPRECT This, LPRGNDATA riid, LPDWORD ppvObj);
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, HWND* This);
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, LPDIRECTDRAW This, DWORD riid);
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, BOOL* This);
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, LPRGNDATA This, DWORD riid);
  HRESULT (__stdcall *)(IDirectDrawClipper* QueryInterface, DWORD This, HWND riid);
};

tagRECT*;

_RGNDATA*;

struct _RGNDATA
{
  RGNDATAHEADER rdh;
  int8[1] Buffer;
};

struct _RGNDATAHEADER { };

struct _RGNDATAHEADER
{
  DWORD dwSize;
  DWORD iType;
  DWORD nCount;
  DWORD nRgnSize;
  RECT rcBound;
};

DWORD*;

IDirectDraw*;

tagPALETTEENTRY*;

struct tagPALETTEENTRY
{
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
};

IDirectDrawPalette*;

struct IDirectDrawPalette
{
  IDirectDrawPaletteVtbl* lpVtbl;
};

struct IDirectDrawPaletteVtbl
{
  HRESULT (__stdcall *)(IDirectDrawPalette* QueryInterface, IID* This, LPVOID* riid);
  ULONG (__stdcall *)(IDirectDrawPalette* QueryInterface);
  ULONG (__stdcall *)(IDirectDrawPalette* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawPalette* QueryInterface, LPDWORD This);
  HRESULT (__stdcall *)(IDirectDrawPalette* QueryInterface, DWORD This, DWORD riid, DWORD ppvObj, LPPALETTEENTRY AddRef);
  HRESULT (__stdcall *)(IDirectDrawPalette* QueryInterface, LPDIRECTDRAW This, DWORD riid, LPPALETTEENTRY ppvObj);
  HRESULT (__stdcall *)(IDirectDrawPalette* QueryInterface, DWORD This, DWORD riid, DWORD ppvObj, LPPALETTEENTRY AddRef);
};

_DDSURFACEDESC*;

struct _DDSURFACEDESC
{
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwHeight;
  DWORD dwWidth;
  $F9D0D49E746EA05C6F8F62A8D439C7A9 dwBackBufferCount;
  DWORD dwAlphaBitDepth;
  $732C1078520B5FCBD2DC52BA2F31A7C8 dwReserved;
  DWORD lpSurface;
  DWORD ddckCKDestOverlay;
  LPVOID ddckCKDestBlt;
  DDCOLORKEY ddckCKSrcOverlay;
  DDCOLORKEY ddckCKSrcBlt;
  DDCOLORKEY ddpfPixelFormat;
  DDCOLORKEY ddsCaps;
  DDPIXELFORMAT ;
  DDSCAPS ;
};

IDirectDrawSurface4*;

HRESULT (__stdcall *LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);

HRESULT (__stdcall *LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE4, LPDDSURFACEDESC, LPVOID);

DDCAPS*;

typedef DDCAPS DDCAPS_DX7;

struct _DDCAPS_DX7 { };

struct _DDCAPS_DX7
{
  DWORD dwSize;
  DWORD dwCaps;
  DWORD dwCaps2;
  DWORD dwCKeyCaps;
  DWORD dwFXCaps;
  DWORD dwFXAlphaCaps;
  DWORD dwPalCaps;
  DWORD dwSVCaps;
  DWORD dwAlphaBltConstBitDepths;
  DWORD dwAlphaBltPixelBitDepths;
  DWORD dwAlphaBltSurfaceBitDepths;
  DWORD dwAlphaOverlayConstBitDepths;
  DWORD dwAlphaOverlayPixelBitDepths;
  DWORD dwAlphaOverlaySurfaceBitDepths;
  DWORD dwZBufferBitDepths;
  DWORD dwVidMemTotal;
  DWORD dwVidMemFree;
  DWORD dwMaxVisibleOverlays;
  DWORD dwCurrVisibleOverlays;
  DWORD dwNumFourCCCodes;
  DWORD dwAlignBoundarySrc;
  DWORD dwAlignSizeSrc;
  DWORD dwAlignBoundaryDest;
  DWORD dwAlignSizeDest;
  DWORD dwAlignStrideAlign;
  DWORD[8] dwRops;
  DDSCAPS ddsOldCaps;
  DWORD dwMinOverlayStretch;
  DWORD dwMaxOverlayStretch;
  DWORD dwMinLiveVideoStretch;
  DWORD dwMaxLiveVideoStretch;
  DWORD dwMinHwCodecStretch;
  DWORD dwMaxHwCodecStretch;
  DWORD dwReserved1;
  DWORD dwReserved2;
  DWORD dwReserved3;
  DWORD dwSVBCaps;
  DWORD dwSVBCKeyCaps;
  DWORD dwSVBFXCaps;
  DWORD[8] dwSVBRops;
  DWORD dwVSBCaps;
  DWORD dwVSBCKeyCaps;
  DWORD dwVSBFXCaps;
  DWORD[8] dwVSBRops;
  DWORD dwSSBCaps;
  DWORD dwSSBCKeyCaps;
  DWORD dwSSBFXCaps;
  DWORD[8] dwSSBRops;
  DWORD dwMaxVideoPorts;
  DWORD dwCurrVideoPorts;
  DWORD dwSVBCaps2;
  DWORD dwNLVBCaps;
  DWORD dwNLVBCaps2;
  DWORD dwNLVBCKeyCaps;
  DWORD dwNLVBFXCaps;
  DWORD[8] dwNLVBRops;
  DDSCAPS2 ddsCaps;
};

BOOL*;

void*;

DDSCAPS2*;

tagDDDEVICEIDENTIFIER*;

struct tagDDDEVICEIDENTIFIER
{
  int8[512] szDriver;
  int8[512] szDescription;
  LARGE_INTEGER liDriverVersion;
  DWORD dwVendorId;
  DWORD dwDeviceId;
  DWORD dwSubSysId;
  DWORD dwRevision;
  GUID guidDeviceIdentifier;
};

union _LARGE_INTEGER { };

union _LARGE_INTEGER
{
  _LARGE_INTEGER::$837407842DC9087486FDFA5FEB63B74E u;
  _LARGE_INTEGER::$837407842DC9087486FDFA5FEB63B74E QuadPart;
  LONGLONG ;
};

struct _LARGE_INTEGER::$837407842DC9087486FDFA5FEB63B74E
{
  DWORD LowPart;
  LONG HighPart;
};

int64;

struct PcxData
{
  int PCXvers;
  uint width;
  uint length;
  int BPPixel;
  int Nplanes;
  int BytesPerLine;
  int PalInt;
  int vbitcount;
};

struct RGBQUAD
{
  uchar rgbBlue;
  uchar rgbGreen;
  uchar rgbRed;
  uchar rgbReserved;
};

struct BITMAPINFOHEADER
{
  uint biSize;
  int biWidth;
  int biHeight;
  ushort biPlanes;
  ushort biBitCount;
  uint biCompression;
  uint biSizeImage;
  int biXPelsPerMeter;
  int biYPelsPerMeter;
  uint biClrUsed;
  uint biClrImportant;
};

struct imgdes
{
  uchar* ibuff;
  uint stx;
  uint sty;
  uint endx;
  uint endy;
  uint buffwidth;
  RGBQUAD* palette;
  int colors;
  int imgtype;
  BITMAPINFOHEADER* bmh;
  void* hBitmap;
};

struct std::vector_string_
{
  int8 allocator;
  std::string* first;
  std::string* last;
  std::string* end;
};

struct t_scenario_start_options
{
  t_scenario_start_options::vftable_t* vftable;
};

struct t_start_hero_options : t_scenario_start_options
{
  std::vector_crossover_hero_ options;
};

struct t_crossover_options : t_scenario_start_options
{
  std::vector_crossover_option_ options;
};

struct TCampaignBrief::MapTextStruct
{
  int video;
  int audio;
  std::string subtitles;
};

struct std::bitset_145_
{
  byte[20] bits;
};

struct TCampaignBrief::CampaignHeaderStruct
{
  int file_error;
  std::string file_name;
  int campaign_version;
  int region_map;
  std::string campaign_name;
  std::string campaign_desc;
  std::vector_TCampaignBrief::ScenarioStruct_ptr_ scenarios;
  uchar* data;
  std::streambuf* stream;
  bool variable_difficulty;
  int campaign_music;
};

struct crossover_hero
{
  int player;
  int hero;
};

struct crossover_option
{
  int8 player;
  int8 scenario;
};

struct t_scenario_start_options::vftable_t
{
  void (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, uchar is_image_bitmap);
  bool (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, int32_t is_image_bitmap);
  int (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor);
  int8* (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, int32_t is_image_bitmap);
  int (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, int32_t is_image_bitmap);
  int (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, TCampaignBrief::ScenarioStruct* is_image_bitmap, int get_options_amount);
  std::string* (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, std::string* is_image_bitmap, TCampaignBrief::CampaignHeaderStruct* get_options_amount, int get_image_name);
  int (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, int is_image_bitmap);
  int (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, int is_image_bitmap);
  void (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, TAbstractFile* is_image_bitmap);
  void (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, TCampaignBrief::ScenarioStruct* is_image_bitmap);
  void (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, NewSMapHeader* is_image_bitmap);
  bool (__thiscall *)(t_scenario_start_options* scalar_deleting_destructor, TCampaignBrief::ScenarioStruct* is_image_bitmap, int get_options_amount);
};

struct t_start_bonus
{
  t_start_bonus::vftable_t* vftable;
};

struct t_start_bonus::vftable_t
{
  void (__thiscall *)(t_start_bonus* scalar_deleting_destructor, uchar is_image_bitmap);
  bool (__thiscall *)(t_start_bonus* scalar_deleting_destructor);
  int8* (__thiscall *)(t_start_bonus* scalar_deleting_destructor);
  int (__thiscall *)(t_start_bonus* scalar_deleting_destructor);
  std::string* (__thiscall *)(t_start_bonus* scalar_deleting_destructor, std::string* is_image_bitmap);
  void (__thiscall *)(t_start_bonus* scalar_deleting_destructor, int is_image_bitmap);
  void (__thiscall *)(t_start_bonus* scalar_deleting_destructor, TAbstractFile* is_image_bitmap);
  void (__thiscall *)(t_start_bonus* scalar_deleting_destructor, TTownType is_image_bitmap);
};

struct t_start_resource_bonus : t_start_bonus
{
  EGameResource resource;
  int amount;
};

struct t_start_secondary_skill_bonus : t_start_bonus
{
  THeroID hero;
  TSecondarySkill skill;
  TSkillMastery mastery;
};

struct t_start_primary_skill_bonus : t_start_bonus
{
  THeroID hero;
  int8[4] skills;
};

struct t_start_artifact_bonus : t_start_bonus
{
  THeroID hero;
  TArtifact artifact;
};

struct t_start_building_bonus : t_start_bonus
{
  TTownType town;
  type_building_id building;
};

struct t_start_creature_bonus : t_start_bonus
{
  THeroID hero;
  TCreatureType type;
  int amount;
};

struct t_start_spell_scroll_bonus : t_start_bonus
{
  THeroID hero;
  SpellID spell;
};

struct t_start_spell_bonus : t_start_bonus
{
  THeroID hero;
  SpellID spell;
};

struct std::vector_crossover_option_
{
  int8 allocator;
  crossover_option* first;
  crossover_option* last;
  crossover_option* end;
};

struct std::vector_crossover_hero_
{
  int8 allocator;
  crossover_hero* first;
  crossover_hero* last;
  crossover_hero* end;
};

struct std::vector_t_start_bonus_ptr_
{
  int8 allocator;
  t_start_bonus** first;
  t_start_bonus** last;
  t_start_bonus** end;
};

struct t_bonus_options : t_scenario_start_options
{
  int player;
  std::vector_t_start_bonus_ptr_ bonuses;
};

struct CampaignScenarioPreview : NewSMapHeader
{
  SGameSetupOptions game_setup;
  bool available;
};

struct std::vector_CampaignScenarioPreview_
{
  int8 allocator;
  CampaignScenarioPreview* first;
  CampaignScenarioPreview* last;
  CampaignScenarioPreview* end;
};

struct std::logic_error : std::exception
{
  std::string _Str;
};

struct std::streambuf::vftable_t
{
  void (__thiscall *)(std::streambuf* scalar_deleting_destructor, bool overflow);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor, int overflow);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor, int overflow);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor, int8* overflow, int pbackfail);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor, int8* overflow, int pbackfail);
  fpos_int_* (__thiscall *)(std::streambuf* scalar_deleting_destructor, fpos_int_* overflow, int pbackfail, int showmanyc, int underflow);
  fpos_int_* (__thiscall *)(std::streambuf* scalar_deleting_destructor, fpos_int_* overflow, int pbackfail, int showmanyc);
  std::streambuf* (__thiscall *)(std::streambuf* scalar_deleting_destructor, int8* overflow, int pbackfail);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor);
  int (__thiscall *)(std::streambuf* scalar_deleting_destructor, void* overflow);
};

struct std::filebuf : std::streambuf
{
  void* _Pcvt;
  int _State0;
  int _State;
  std::string* _Str;
  bool _Closef;
  void* _Loc;
  FILE* _File;
};

enum ios_base::openmode
{
  ios_base__in = 0x1,
  ios_base__out = 0x2,
  ios_base__ate = 0x4,
  ios_base__app = 0x8,
  ios_base__trunc = 0x10,
  ios_base__binary = 0x20,
};

enum ios_base::seekdir
{
  ios_base__beg = 0x0,
  ios_base__cur = 0x1,
  ios_base__end = 0x2,
};

enum ios_base::iostate
{
  ios_base__goodbit = 0x0,
  ios_base__eofbit = 0x1,
  ios_base__failbit = 0x2,
  ios_base__badbit = 0x4,
  ios_base___Statmask = 0x7,
};

struct std::vector_TCampaignBrief::ScenarioStruct_ptr_
{
  int8 allocator;
  TCampaignBrief::ScenarioStruct* first;
  TCampaignBrief::ScenarioStruct* last;
  TCampaignBrief::ScenarioStruct* end;
};

struct tree_ulong_
{
  int count_entries;
  int* names;
};

struct SoundHeaderStruct
{
  int8[40] filename;
  int offset;
  int size;
};

typedef EHRegistrationNode ULONG;

struct EHExceptionRecord::EHParameters
{
  unsigned int magicNumber;
  void* pExceptionObject;
  ThrowInfo* pThrowInfo;
};

struct EHExceptionRecord
{
  unsigned int ExceptionCode;
  unsigned int ExceptionFlags;
  _EXCEPTION_RECORD* ExceptionRecord;
  void* ExceptionAddress;
  unsigned int NumberParameters;
  EHExceptionRecord::EHParameters params;
};

struct Context
{
  void* vftable;
};

struct PMD
{
  int mdisp;
  int pdisp;
  int vdisp;
};

struct CatchableType
{
  unsigned int properties;
  TypeDescriptor* pType;
  PMD thisDisplacement;
  int sizeOrOffset;
  void (__cdecl *)();
};

struct CatchableTypeArray
{
  int nCatchableTypes;
  CatchableType*[] arrayOfCatchableTypes;
};

struct ThrowInfo
{
  unsigned int attributes;
  void (__cdecl *)();
  int (*)(void* attributes);
  CatchableTypeArray* args;
};

struct FuncInfo
{
  int magicNumber;
  int maxState;
  void* pUnwindMap;
  int nTryBlocks;
  void* pTryBlockMap;
  int nIPMapEntries;
  void* pIPtoStateMap;
  void* pESTypeList;
  int EHFlags;
};

struct std::exception::vftable_t
{
  void (__thiscall *)(std::exception* scalar_deleting_destructor, uchar this);
  int8* (__thiscall *)(std::exception* scalar_deleting_destructor);
  void (__thiscall *)(std::exception* scalar_deleting_destructor);
};

struct std::domain_error : std::logic_error { };

struct std::invalid_argument : std::logic_error { };

struct std::out_of_range : std::logic_error { };

struct std::runtime_error : std::exception
{
  std::string _Str;
};

struct std::overflow_error : std::runtime_error { };

struct std::underflow_error : std::runtime_error { };

struct std::range_error : std::runtime_error { };

struct _EXCEPTION_POINTERS
{
  PEXCEPTION_RECORD ExceptionRecord;
  PCONTEXT ContextRecord;
};

EXCEPTION_RECORD*;

CONTEXT*;

struct _CONTEXT { };

struct _CONTEXT
{
  DWORD ContextFlags;
  DWORD Dr0;
  DWORD Dr1;
  DWORD Dr2;
  DWORD Dr3;
  DWORD Dr6;
  DWORD Dr7;
  FLOATING_SAVE_AREA FloatSave;
  DWORD SegGs;
  DWORD SegFs;
  DWORD SegEs;
  DWORD SegDs;
  DWORD Edi;
  DWORD Esi;
  DWORD Ebx;
  DWORD Edx;
  DWORD Ecx;
  DWORD Eax;
  DWORD Ebp;
  DWORD Eip;
  DWORD SegCs;
  DWORD EFlags;
  DWORD Esp;
  DWORD SegSs;
  BYTE[512] ExtendedRegisters;
};

struct _FLOATING_SAVE_AREA { };

struct _FLOATING_SAVE_AREA
{
  DWORD ControlWord;
  DWORD StatusWord;
  DWORD TagWord;
  DWORD ErrorOffset;
  DWORD ErrorSelector;
  DWORD DataOffset;
  DWORD DataSelector;
  BYTE[80] RegisterArea;
  DWORD Spare0;
};

struct TranslatorGuardRN
{
  EHRegistrationNode* pNext;
  void* pFrameHandler;
  FuncInfo* pFuncInfo;
  EHRegistrationNode* pRN;
  int CatchDepth;
  EHRegistrationNode* pMarkerRN;
  EHRegistrationNode* jumpToNode;
  void* ESP;
  void* EBP;
  int DidUnwind;
};

void (__cdecl *_se_translator_function)(unsigned int, _EXCEPTION_POINTERS*);

struct _TEB
{
  _NT_TIB NtTib;
  void* EnvironmentPointer;
  _CLIENT_ID ClientId;
  void* ActiveRpcHandle;
  void* ThreadLocalStoragePointer;
  _PEB* ProcessEnvironmentBlock;
  unsigned int LastErrorValue;
  unsigned int CountOfOwnedCriticalSections;
  void* CsrClientThread;
  void* Win32ThreadInfo;
  unsigned int[26] User32Reserved;
  unsigned int[5] UserReserved;
  void* WOW32Reserved;
  unsigned int CurrentLocale;
  unsigned int FpSoftwareStatusRegister;
  void*[54] SystemReserved1;
  int ExceptionCode;
  _ACTIVATION_CONTEXT_STACK* ActivationContextStackPointer;
  uint8_t[36] SpareBytes;
  unsigned int TxFsContext;
  _GDI_TEB_BATCH GdiTebBatch;
  _CLIENT_ID RealClientId;
  void* GdiCachedProcessHandle;
  unsigned int GdiClientPID;
  unsigned int GdiClientTID;
  void* GdiThreadLocalInfo;
  unsigned int[62] Win32ClientInfo;
  void*[233] glDispatchTable;
  unsigned int[29] glReserved1;
  void* glReserved2;
  void* glSectionInfo;
  void* glSection;
  void* glTable;
  void* glCurrentRC;
  void* glContext;
  unsigned int LastStatusValue;
  _UNICODE_STRING StaticUnicodeString;
  wchar_t[261] StaticUnicodeBuffer;
  void* DeallocationStack;
  void*[64] TlsSlots;
  _LIST_ENTRY TlsLinks;
  void* Vdm;
  void* ReservedForNtRpc;
  void*[2] DbgSsReserved;
  unsigned int HardErrorMode;
  void*[9] Instrumentation;
  _GUID ActivityId;
  void* SubProcessTag;
  void* EtwLocalData;
  void* EtwTraceData;
  void* WinSockData;
  unsigned int GdiBatchCount;
  _TEB::$A3D02A70492DFE9D91413B66511C1D96 ReservedPad1;
  uint8_t ReservedPad2;
  uint8_t IdealProcessor;
  uint8_t GuaranteedStackBytes;
  unsigned int ReservedForPerf;
  void* ReservedForOle;
  void* WaitingOnLoaderLock;
  unsigned int SavedPriorityState;
  void* SoftPatchPtr1;
  unsigned int ThreadPoolData;
  void* TlsExpansionSlots;
  void** MuiGeneration;
  unsigned int IsImpersonating;
  unsigned int NlsCache;
  void* pShimData;
  void* HeapVirtualAffinity;
  unsigned int CurrentTransactionHandle;
  void* ActiveFrame;
  _TEB_ACTIVE_FRAME* FlsData;
  void* PreferredLanguages;
  void* UserPrefLanguages;
  void* MergedPrefLanguages;
  void* MuiImpersonation;
  unsigned int TxnScopeEnterCallback;
  _TEB::$9CF806A5F7AA4F50D4778A9253E08EA3 TxnScopeExitCallback;
  _TEB::$368A8F43BCCCFC17E5DBBF98016F1166 TxnScopeContext;
  void* LockCount;
  void* SpareUlong0;
  void* ResourceRetValue;
  unsigned int ;
  unsigned int ;
  void* ;
};

struct _NT_TIB
{
  _EXCEPTION_REGISTRATION_RECORD* ExceptionList;
  PVOID StackBase;
  PVOID StackLimit;
  PVOID SubSystemTib;
  _NT_TIB::$0349ADB4452EC09BEC08E2292695FBBA ArbitraryUserPointer;
  PVOID Self;
  _NT_TIB* ;
};

struct _EXCEPTION_REGISTRATION_RECORD
{
  _EXCEPTION_REGISTRATION_RECORD* Next;
  PEXCEPTION_ROUTINE Handler;
};

EXCEPTION_ROUTINE*;

EXCEPTION_DISPOSITION (__stdcall EXCEPTION_ROUTINE)(_EXCEPTION_RECORD* ExceptionRecord, PVOID EstablisherFrame, _CONTEXT* ContextRecord, PVOID DispatcherContext);

enum EXCEPTION_DISPOSITION
{
};

enum _EXCEPTION_DISPOSITION
{
  ExceptionContinueExecution = 0x0,
  ExceptionContinueSearch = 0x1,
  ExceptionNestedException = 0x2,
  ExceptionCollidedUnwind = 0x3,
};

union _NT_TIB::$0349ADB4452EC09BEC08E2292695FBBA
{
  PVOID FiberData;
  DWORD Version;
};

struct _CLIENT_ID
{
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
};

struct _PEB
{
  uint8_t InheritedAddressSpace;
  uint8_t ReadImageFileExecOptions;
  uint8_t BeingDebugged;
  _PEB::$D57935FE5756AF9F9B84A66E67E8019A Mutant;
  void* ImageBaseAddress;
  void* Ldr;
  _PEB_LDR_DATA* ProcessParameters;
  _RTL_USER_PROCESS_PARAMETERS* SubSystemData;
  void* ProcessHeap;
  void* FastPebLock;
  _RTL_CRITICAL_SECTION* AtlThunkSListPtr;
  void* IFEOKey;
  void* SystemReserved;
  _PEB::$9091FB23ACFC48B9D2023E9670FB1584 AtlThunkSListPtr32;
  _PEB::$6F1CA9A36B21C857AE5467E073440320 ApiSetMap;
  unsigned int[1] TlsExpansionCounter;
  unsigned int TlsBitmap;
  void* TlsBitmapBits;
  unsigned int ReadOnlySharedMemoryBase;
  void* HotpatchInformation;
  unsigned int[2] ReadOnlyStaticServerData;
  void* AnsiCodePageData;
  void* OemCodePageData;
  void** UnicodeCaseTableData;
  void* NumberOfProcessors;
  void* NtGlobalFlag;
  void* CriticalSectionTimeout;
  unsigned int HeapSegmentReserve;
  unsigned int HeapSegmentCommit;
  _LARGE_INTEGER HeapDeCommitTotalFreeThreshold;
  unsigned int HeapDeCommitFreeBlockThreshold;
  unsigned int NumberOfHeaps;
  unsigned int MaximumNumberOfHeaps;
  unsigned int ProcessHeaps;
  unsigned int GdiSharedHandleTable;
  unsigned int ProcessStarterHelper;
  void** GdiDCAttributeList;
  void* LoaderLock;
  void* OSMajorVersion;
  unsigned int OSMinorVersion;
  _RTL_CRITICAL_SECTION* OSBuildNumber;
  unsigned int OSCSDVersion;
  unsigned int OSPlatformId;
  uint16_t ImageSubsystem;
  uint16_t ImageSubsystemMajorVersion;
  unsigned int ImageSubsystemMinorVersion;
  unsigned int ActiveProcessAffinityMask;
  unsigned int GdiHandleBuffer;
  unsigned int PostProcessInitRoutine;
  unsigned int TlsExpansionBitmap;
  unsigned int[34] TlsExpansionBitmapBits;
  void* SessionId;
  void* AppCompatFlags;
  unsigned int[32] AppCompatFlagsUser;
  unsigned int pShimData;
  _ULARGE_INTEGER AppCompatInfo;
  _ULARGE_INTEGER CSDVersion;
  void* ActivationContextData;
  void* ProcessAssemblyStorageMap;
  _UNICODE_STRING SystemDefaultActivationContextData;
  _ACTIVATION_CONTEXT_DATA* SystemAssemblyStorageMap;
  _ASSEMBLY_STORAGE_MAP* MinimumStackCommit;
  _ACTIVATION_CONTEXT_DATA* FlsCallback;
  _ASSEMBLY_STORAGE_MAP* FlsListHead;
  unsigned int FlsBitmap;
  _FLS_CALLBACK_INFO* FlsBitmapBits;
  _LIST_ENTRY FlsHighIndex;
  void* WerRegistrationData;
  unsigned int[4] WerShipAssertPtr;
  unsigned int pContextData;
  void* pImageHeaderHash;
  void* ;
  void* ;
  void* ;
  _PEB::$FE4C172558F245B1558E1D5462D60311 ;
};

unsigned int8;

union _PEB::$D57935FE5756AF9F9B84A66E67E8019A
{
  uint8_t BitField;
  _PEB::$D57935FE5756AF9F9B84A66E67E8019A::$FD42647A95697634BDFB73B2D6521EB2 ;
};

struct _PEB::$D57935FE5756AF9F9B84A66E67E8019A::$FD42647A95697634BDFB73B2D6521EB2
{
  unsigned int8 ImageUsesLargePages : 1;
  unsigned int8 IsProtectedProcess : 1;
  unsigned int8 IsLegacyProcess : 1;
  unsigned int8 IsImageDynamicallyRelocated : 1;
  unsigned int8 SkipPatchingUser32Forwarders : 1;
  unsigned int8 SpareBits : 3;
};

struct _PEB_LDR_DATA
{
  unsigned int Length;
  uint8_t Initialized;
  void* SsHandle;
  _LIST_ENTRY InLoadOrderModuleList;
  _LIST_ENTRY InMemoryOrderModuleList;
  _LIST_ENTRY InInitializationOrderModuleList;
  void* EntryInProgress;
  uint8_t ShutdownInProgress;
  void* ShutdownThreadId;
};

struct _RTL_USER_PROCESS_PARAMETERS
{
  unsigned int MaximumLength;
  unsigned int Length;
  unsigned int Flags;
  unsigned int DebugFlags;
  void* ConsoleHandle;
  unsigned int ConsoleFlags;
  void* StandardInput;
  void* StandardOutput;
  void* StandardError;
  _CURDIR CurrentDirectory;
  _UNICODE_STRING DllPath;
  _UNICODE_STRING ImagePathName;
  _UNICODE_STRING CommandLine;
  void* Environment;
  unsigned int StartingX;
  unsigned int StartingY;
  unsigned int CountX;
  unsigned int CountY;
  unsigned int CountCharsX;
  unsigned int CountCharsY;
  unsigned int FillAttribute;
  unsigned int WindowFlags;
  unsigned int ShowWindowFlags;
  _UNICODE_STRING WindowTitle;
  _UNICODE_STRING DesktopInfo;
  _UNICODE_STRING ShellInfo;
  _UNICODE_STRING RuntimeData;
  _RTL_DRIVE_LETTER_CURDIR[32] CurrentDirectores;
  unsigned int EnvironmentSize;
  unsigned int EnvironmentVersion;
};

struct _CURDIR
{
  _UNICODE_STRING DosPath;
  void* Handle;
};

struct _UNICODE_STRING
{
  unsigned int16 Length;
  unsigned int16 MaximumLength;
  wchar_t* Buffer;
};

struct _RTL_DRIVE_LETTER_CURDIR
{
  uint16_t Flags;
  uint16_t Length;
  unsigned int TimeStamp;
  _STRING DosPath;
};

unsigned int16;

struct _STRING
{
  unsigned int16 Length;
  unsigned int16 MaximumLength;
  int8* Buffer;
};

union _PEB::$9091FB23ACFC48B9D2023E9670FB1584
{
  unsigned int CrossProcessFlags;
  _PEB::$9091FB23ACFC48B9D2023E9670FB1584::$B5C0D3AF06C7C0E0C35F2AF79ED7DD00 ;
};

struct _PEB::$9091FB23ACFC48B9D2023E9670FB1584::$B5C0D3AF06C7C0E0C35F2AF79ED7DD00
{
  unsigned int32 ProcessInJob : 1;
  unsigned int32 ProcessInitializing : 1;
  unsigned int32 ProcessUsingVEH : 1;
  unsigned int32 ProcessUsingVCH : 1;
  unsigned int32 ProcessUsingFTH : 1;
  unsigned int32 ReservedBits0 : 27;
};

union _PEB::$6F1CA9A36B21C857AE5467E073440320
{
  void* KernelCallbackTable;
  void* UserSharedInfoPtr;
};

union _ULARGE_INTEGER
{
  _ULARGE_INTEGER::$0354AA9C204208F00D0965D07BBE7FAC u;
  _ULARGE_INTEGER::$0354AA9C204208F00D0965D07BBE7FAC QuadPart;
  ULONGLONG ;
};

struct _ULARGE_INTEGER::$0354AA9C204208F00D0965D07BBE7FAC
{
  DWORD LowPart;
  DWORD HighPart;
};

unsigned int64;

union _PEB::$FE4C172558F245B1558E1D5462D60311
{
  unsigned int TracingFlags;
  _PEB::$FE4C172558F245B1558E1D5462D60311::$1E4969F9739BBB4BED3E38EB5AAA4AF7 ;
};

struct _PEB::$FE4C172558F245B1558E1D5462D60311::$1E4969F9739BBB4BED3E38EB5AAA4AF7
{
  unsigned int32 HeapTracingEnabled : 1;
  unsigned int32 CritSecTracingEnabled : 1;
  unsigned int32 SpareTracingBits : 30;
};

struct _ACTIVATION_CONTEXT_STACK
{
  _RTL_ACTIVATION_CONTEXT_STACK_FRAME* ActiveFrame;
  _LIST_ENTRY FrameListCache;
  unsigned int Flags;
  unsigned int NextCookieSequenceNumber;
  unsigned int StackId;
};

struct _GDI_TEB_BATCH
{
  unsigned int Offset;
  unsigned int HDC;
  unsigned int[310] Buffer;
};

union _TEB::$A3D02A70492DFE9D91413B66511C1D96
{
  _PROCESSOR_NUMBER CurrentIdealProcessor;
  unsigned int IdealProcessorValue;
  uint8_t ReservedPad0;
};

struct _PROCESSOR_NUMBER
{
  WORD Group;
  BYTE Number;
  BYTE Reserved;
};

union _TEB::$9CF806A5F7AA4F50D4778A9253E08EA3
{
  uint16_t CrossTebFlags;
  _TEB::$9CF806A5F7AA4F50D4778A9253E08EA3::$88D35C6E749BA8930BA8A8A22D5F60D0 ;
};

struct _TEB::$9CF806A5F7AA4F50D4778A9253E08EA3::$88D35C6E749BA8930BA8A8A22D5F60D0
{
  unsigned int16 SpareCrossTebBits : 16;
};

union _TEB::$368A8F43BCCCFC17E5DBBF98016F1166
{
  uint16_t SameTebFlags;
  _TEB::$368A8F43BCCCFC17E5DBBF98016F1166::$DB2E6D00F02C708C0B2EF262B4D055F5 ;
};

struct _TEB::$368A8F43BCCCFC17E5DBBF98016F1166::$DB2E6D00F02C708C0B2EF262B4D055F5
{
  unsigned int16 SafeThunkCall : 1;
  unsigned int16 InDebugPrint : 1;
  unsigned int16 HasFiberData : 1;
  unsigned int16 SkipThreadAttach : 1;
  unsigned int16 WerInShipAssertCode : 1;
  unsigned int16 RanProcessInit : 1;
  unsigned int16 ClonedThread : 1;
  unsigned int16 SuppressDebugMsg : 1;
  unsigned int16 DisableUserStackWalk : 1;
  unsigned int16 RtlExceptionAttached : 1;
  unsigned int16 InitialThread : 1;
  unsigned int16 SpareSameTebBits : 5;
};

struct exception { };

struct _s_CatchableType
{
  unsigned int properties;
  TypeDescriptor* pType;
  PMD thisDisplacement;
  int sizeOrOffset;
  PMFN copyFunction;
};

void (__cdecl *PMFN)(void*);

struct textButton : button
{
  font* Font;
  font::TColor textColor;
  Bitmap16Bit* textImage;
};

struct CHeroSessions : CAutoArray_CDPlaySession_ { };

struct TMultiPlayerWindow : CHeroWindowEx
{
  CSprite* GameState;
  bool inSessionList;
  bool showSplash;
  int currentGame;
  int currentIndex;
  CHeroSessions* pSessions;
  uint sessTimer;
  uint sessionRefreshTimeout;
  int8[80] localIPAddress;
  textEntryWidget* playerName;
  bool hostJoinScreen;
  bitmapBorder* splash;
  button* hotSeat;
  button* ipx;
  button* tcp;
  button* modem;
  button* direct;
  button* online;
  button* host;
  button* join;
  button* search;
  button* cancel;
  slider* gameSlider;
  textWidget* sessNameHeader;
  textWidget* userNameHeader;
  textWidget* RolloverWidget;
};

struct _tiddata
{
  unsigned int _tid;
  unsigned int _thandle;
  int _terrno;
  unsigned int _tdoserrno;
  unsigned int _fpds;
  unsigned int _holdrand;
  int8* _token;
  wchar_t* _wtoken;
  unsigned int8* _mtoken;
  int8* _errmsg;
  int8* _namebuf0;
  wchar_t* _wnamebuf0;
  int8* _namebuf1;
  wchar_t* _wnamebuf1;
  int8* _asctimebuf;
  wchar_t* _wasctimebuf;
  void* _gmtimebuf;
  int8* _cvtbuf;
  void* _initaddr;
  void* _initarg;
  _XCPT_ACTION* _pxcptacttab;
  void* _tpxcptinfoptrs;
  int _tfpecode;
  unsigned int _NLG_dwCode;
  void (*)();
  void* _unexpected;
  void (__cdecl *)(unsigned int _tid, _EXCEPTION_POINTERS* _thandle);
  void* _curexception;
  void* _curcontext;
};

_tiddata*;

struct errentry
{
  unsigned int oscode;
  int errnocode;
};

void (__cdecl *_PHNDLR)(int);

struct _XCPT_ACTION
{
  unsigned int XcptNum;
  int SigNum;
  _PHNDLR XcptAction;
};

struct type_creature_bank_traits
{
  std::string name;
  type_creature_bank_level[4] levels;
};

struct CampaignScenarioInfo
{
  bool completed;
  int days;
  int score;
  int index;
  int complete_order;
};

struct std::vector_CampaignScenarioInfo_
{
  int8 allocator;
  CampaignScenarioInfo* first;
  CampaignScenarioInfo* last;
  CampaignScenarioInfo* end;
};

struct std::vector_hero_
{
  int8 allocator;
  hero* first;
  hero* last;
  hero* end;
};

struct std::vector_type_artifact_
{
  int8 allocator;
  type_artifact* first;
  type_artifact* last;
  type_artifact* end;
};

struct std::vector_vector__hero__
{
  int8 allocator;
  std::vector_hero_* first;
  std::vector_hero_* last;
  std::vector_hero_* end;
};

struct std::vector_vector__type_artifact__
{
  int8 allocator;
  std::vector_type_artifact_* first;
  std::vector_type_artifact_* last;
  std::vector_type_artifact_* end;
};

struct combatManager::TArcherTraits
{
  TCreatureType CreatureType;
  int MainBuildingX;
  int MainBuildingY;
  int LowerTowerX;
  int LowerTowerY;
  int UpperTowerX;
  int UpperTowerY;
  int8* MissileName;
};

enum ECampaignType
{
  ROE_LONG_LIVE_THE_QUEEN = 0x0,
  ROE_LIBERATION = 0x1,
  ROE_SPOILS_OF_WAR = 0x2,
  ROE_SONG_FOR_THE_FATHER = 0x3,
  ROE_DUNGEONS_AND_DEVILS = 0x4,
  ROE_LONG_LIVE_THE_KING = 0x5,
  ROE_SEEDS_OF_DISCONTENT = 0x6,
  AB_ARMAGEDDONS_BLADE = 0x7,
  AB_DRAGONS_BLOOD = 0x8,
  AB_DRAGON_SLAYER = 0x9,
  AB_FESTIVAL_OF_LIFE = 0xA,
  AB_FOOLHARDY_WAYWARDNESS = 0xB,
  AB_PLAYING_WITH_FIRE = 0xC,
  SOD_HACK_AND_SLASH = 0xD,
  SOD_BIRTH_OF_A_BARBARIAN = 0xE,
  SOD_NEW_BEGINNING = 0xF,
  SOD_ELIXIR_OF_LIFE = 0x10,
  SOD_RISE_OF_THE_NECROMANCER = 0x11,
  SOD_UNHOLY_ALLIANCE = 0x12,
  SOD_SPECTRE_OF_POWER = 0x13,
  CUSTOM_CAMPAIGN = 0x14,
};

struct IDirectDrawSurfaceVtbl
{
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, IID* This, LPVOID* riid);
  ULONG (__stdcall *)(IDirectDrawSurface* QueryInterface);
  ULONG (__stdcall *)(IDirectDrawSurface* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAWSURFACE This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPRECT This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPRECT This, LPDIRECTDRAWSURFACE riid, LPRECT ppvObj, DWORD AddRef, LPDDBLTFX This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDDBLTBATCH This, DWORD riid, DWORD ppvObj);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This, DWORD riid, LPDIRECTDRAWSURFACE ppvObj, LPRECT AddRef, DWORD This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This, LPDIRECTDRAWSURFACE riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPVOID This, LPDDENUMSURFACESCALLBACK riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This, LPVOID riid, LPDDENUMSURFACESCALLBACK ppvObj);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAWSURFACE This, DWORD riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDDSCAPS This, LPDIRECTDRAWSURFACE* riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDDSCAPS This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAWCLIPPER* This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This, LPDDCOLORKEY riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, HDC* This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPLONG This, LPLONG riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAWPALETTE* This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDDPIXELFORMAT This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDDSURFACEDESC This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAW This, LPDDSURFACEDESC riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPRECT This, LPDDSURFACEDESC riid, DWORD ppvObj, HANDLE AddRef);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, HDC This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAWCLIPPER This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This, LPDDCOLORKEY riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LONG This, LONG riid);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPDIRECTDRAWPALETTE This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPVOID This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, LPRECT This, LPDIRECTDRAWSURFACE riid, LPRECT ppvObj, DWORD AddRef, LPDDOVERLAYFX This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This);
  HRESULT (__stdcall *)(IDirectDrawSurface* QueryInterface, DWORD This, LPDIRECTDRAWSURFACE riid);
};

struct IDirectDrawSurface
{
  IDirectDrawSurfaceVtbl* lpVtbl;
};

IDirectDrawSurface*;

DDBLTFX*;

struct _DDBLTFX { };

DDBLTBATCH*;

struct _DDBLTBATCH { };

struct _DDBLTBATCH
{
  LPRECT lprDest;
  LPDIRECTDRAWSURFACE lpDDSSrc;
  LPRECT lprSrc;
  DWORD dwFlags;
  LPDDBLTFX lpDDBltFx;
};

DDSCAPS*;

DDCOLORKEY*;

int*;

DDPIXELFORMAT*;

DDOVERLAYFX*;

struct _DDOVERLAYFX { };

struct _DDOVERLAYFX
{
  DWORD dwSize;
  DWORD dwAlphaEdgeBlendBitDepth;
  DWORD dwAlphaEdgeBlend;
  DWORD dwReserved;
  DWORD dwAlphaDestConstBitDepth;
  _DDOVERLAYFX::$3639C047B3E88B857708792B1F3FEC83 dwAlphaSrcConstBitDepth;
  DWORD dckDestColorkey;
  _DDOVERLAYFX::$C0828DA952883A43DEF8D462ACC90C87 dckSrcColorkey;
  DDCOLORKEY dwDDFX;
  DDCOLORKEY dwFlags;
  DWORD ;
  DWORD ;
};

union _DDOVERLAYFX::$3639C047B3E88B857708792B1F3FEC83
{
  DWORD dwAlphaDestConst;
  LPDIRECTDRAWSURFACE lpDDSAlphaDest;
};

union _DDOVERLAYFX::$C0828DA952883A43DEF8D462ACC90C87
{
  DWORD dwAlphaSrcConst;
  LPDIRECTDRAWSURFACE lpDDSAlphaSrc;
};

struct TCSLock
{
  LPCRITICAL_SECTION m_lpCriticalSection;
};

struct CHighScoreEdit : textEntryWidget
{
  int unknown1;
  int unknown2;
};

struct CHSInputDlg : CHeroWindowEx
{
  CHighScoreEdit* field1;
  textWidget* header1;
  textWidget* rollover;
};

struct CNetMsgHandler::vftable_t
{
  void (__thiscall *)(resource* scalar_deleting_destructor, bool this);
  CNetMsg* (__thiscall *)(CNetMsgHandler* scalar_deleting_destructor, bool this, bool* flag);
  CNetMsg* (__thiscall *)(CNetMsgHandler* scalar_deleting_destructor);
  CNetMsg* (__thiscall *)(CNetMsgHandler* scalar_deleting_destructor, CNetMsg* this);
  void (__thiscall *)(CNetMsgHandler* scalar_deleting_destructor, CNetMsg* this);
};

struct CPlayerWonMsg : CNetMsg
{
  int m_winner;
  VictoryConditionStruct m_victoryConditionStruct;
};

struct CPlayerLostMsg : CNetMsg
{
  int m_loser;
  LossConditionStruct m_lossConditionStruct;
};

struct CNormalWinMsg : CNetMsg
{
  int m_gamePos;
};

struct CWaitForRemoteBattleDlg : CAnimatedDlg
{
  int unknown1;
  CNetMsgHandlerPause m_pNetMsgHandlerPause;
  bool unknown2;
};

struct TDialogBox::vftable_t : heroWindow::vftable_t
{
  bool (__thiscall *)(TDialogBox* Setup, int this, int, int, int);
};

struct CTextDialog::vftable_t : TDialogBox::vftable_t
{
  bool (__thiscall *)(CTextDialog* Setup, int8* this, font* UpdateText);
  void (__thiscall *)(CTextDialog* Setup, int8* this);
  void (__thiscall *)(CTextDialog* Setup, int8* this, font* UpdateText, int* this, int* CalcDimensions, int* this, int*);
};

union heroWindow::vftable_union_t
{
  heroWindow::vftable_t* heroWindow_vftable;
  TDialogBox::vftable_t* TDialogBox_vftable;
  CTextDialog::vftable_t* CTextDialog_vftable;
  CHeroWindowEx::vftable_t* CHeroWindowEx_vftable_t;
  CAdvPopup::vftable_t* CAdvPopup_vftable;
  CAnimatedDlg::vftable_t* CAnimatedDlg_vftable;
};

struct CHeroWindowEx::vftable_t : heroWindow::vftable_t
{
  int (__thiscall *)(CHeroWindowEx* WindowHandler, message* this);
  bool (__thiscall *)(CHeroWindowEx* WindowHandler, int this, int ProcessHover);
  bool (__thiscall *)(CHeroWindowEx* WindowHandler, int this);
  int (__thiscall *)(CHeroWindowEx* WindowHandler, int this, bool* ProcessHover);
  textWidget* (__thiscall *)(CHeroWindowEx* WindowHandler);
};

struct CAdvPopup::vftable_t : CHeroWindowEx::vftable_t
{
  int (__thiscall *)(CAdvPopup* ExitDialog, message* this);
};

struct fpos_int_
{
  int _Off;
  fpos_t _Fpos;
  int _State;
};

struct std::ios : std::basic_ios { };

struct std::ostream : std::ios
{
  int x_floatused;
};

struct std::ofstream : std::ostream
{
  std::filebuf _Fb;
};

struct t_stdio_file_adapter : TAbstractFile
{
  FILE* file;
};

struct std::bitset_5_
{
  uint bits;
};

struct int64_wrapper_t
{
  int64 value;
};

struct TBuyBuildWindow : CAdvPopup
{
  textWidget* description;
  type_building_id id;
};

struct std::basic_ios : std::ios_base
{
  std::streambuf* _Sb;
  std::ostream* _Tiestr;
  int8 _Fillch;
};

struct t_complex_net_message::vftable_t
{
  bool (__thiscall *)(t_complex_net_message* read, TAbstractFile* this);
  bool (__thiscall *)(t_complex_net_message* read, TAbstractFile* this);
};

struct t_map_list_entry
{
  bool random_map;
  int index;
};

struct CNewPlayerUpdateProc
{
  void* vftable;
  int dpid;
  int numSent;
  std::vector_t_map_list_entry_ map_list;
  int sentTime;
  bool finished;
};

struct std::vector_t_map_list_entry_
{
  int8 allocator;
  t_map_list_entry* first;
  t_map_list_entry* last;
  t_map_list_entry* end;
};

struct CGameHeaderInfoMsg : t_complex_net_message
{
  t_map_list_entry m_map_entry;
  GameSelectionHeadersStruct m_headerInfo;
};

struct std::vector_GameSelectionHeadersStruct_
{
  int8 allocator;
  GameSelectionHeadersStruct* first;
  GameSelectionHeadersStruct* last;
  GameSelectionHeadersStruct* end;
};

struct CSingleSelectionNetMsgHandler : CNetMsgHandler
{
  bool flushMessages;
};

struct CChatSlider : slider { };

struct CChatWidget::CChatSave : Bitmap16Bit
{
  bool screenSaved;
  int m_x;
  int m_y;
};

struct CChatWidget : textWidget
{
  CChatWidget::CChatSave* saveScreen;
};

struct std::deque_CNetMsg_ptr_::iterator
{
  CNetMsg** first;
  CNetMsg** last;
  CNetMsg** next;
  CNetMsg*** map;
};

struct std::deque_CNetMsg_ptr_
{
  int8 allocator;
  std::deque_CNetMsg_ptr_::iterator first;
  std::deque_CNetMsg_ptr_::iterator last;
  CNetMsg* map;
  uint mapsize;
  uint size;
};

struct BinkManager::BinkManagerStruct
{
  BINK* bink;
  BINK* bink2;
  ushort* screen;
  int pitch;
  int height;
  int x;
  int y;
  int w;
  int h;
  int id;
  int loop;
  int paused;
};

enum _unused_enum
{
};

struct CSwapMgrNetMsgHandler : CNetMsgHandler
{
  bool destroyDialog;
};

struct CSortMapsMsg : CNetMsg
{
  int m_how;
  int m_direction;
};

struct CScrollMsg : CNetMsg
{
  int m_currentMap;
  int m_currentIndex;
};

struct CSetFilterMsg : CNetMsg
{
  int m_filterSize;
};

struct CClickMsg : CNetMsg
{
  int m_widgetId;
};

struct CTownUpdateMsg : CNetMsg
{
  int m_gamePos;
  TTownType m_town;
};

struct CRequestHeroFaceMsg : CNetMsg
{
  int m_which;
};

struct CRequestHeroFaceReplyMsg : CNetMsg
{
  int m_pos;
  int m_heroFace;
};

struct CSetAGRMsg : CNetMsg
{
  int m_gamePos;
  int m_agr;
};

struct CMapHeaderRequestMsg : CNetMsg
{
  int m_nbr;
};

struct CUpdatePlayerPosMsg : CNetMsg
{
  CNetPlayerHandlerPlayer[8] m_netPlayer;
  CNetPlayerHandlerPlayer[8] m_compPlayer;
};

struct CDPlayHeroes::vftable_t : CDPlayLobby::vftable_t { };

union CDPlay::vftable_union_t
{
  CDPlay::vftable_t* CDPlay_vftable;
  CDPlayLobby::vftable_t* CDPlayLobby_vftable;
  CDPlayHeroes::vftable_t* CDPlayHeroes_vftable;
};

struct type_skeleton_window : CAdvPopup
{
  textWidget* rollover_widget;
  type_func_button* sacrifice_button;
  type_func_button* all_creatures_button;
  int selected_group;
  int selected_index;
  armyGroup selected_creatures;
  armyGroup*[2] armies;
  iconWidget*[7][2] army_widget;
  iconWidget*[7][2] select_border;
  textWidget*[7][2] army_label;
  std::vector death_samples;
};

struct type_normal_dialog_frame : coloredBorderFrame
{
  EGameResource resource;
  int qualifier;
};

struct type_university_skill
{
  int[5] unknown;
};

enum message::ECommandType
{
  COMMAND_INPUT_KEY_DOWN = 0x1,
  COMMAND_INPUT_KEY_UP = 0x2,
  COMMAND_MOUSE_ROLLOVER = 0x4,
  COMMAND_MOUSE_LEFT_DOWN = 0x8,
  COMMAND_MOUSE_LEFT_UP = 0x10,
  COMMAND_MOUSE_RIGHT_DOWN = 0x20,
  COMMAND_MOUSE_RIGHT_UP = 0x40,
  COMMAND_WINDOW = 0x200,
  COMMAND_MANAGER = 0x4000,
};

struct t_custom_campaign_window : CHeroWindowEx
{
  int[48] unk1;
};

int;

struct _finddata_t
{
  unsigned int attrib;
  time_t time_create;
  time_t time_access;
  time_t time_write;
  size_t size;
  int8[260] name;
};

struct _finddatai64_t
{
  unsigned int attrib;
  time_t time_create;
  time_t time_access;
  time_t time_write;
  int64 size;
  int8[260] name;
};

enum MACRO_ENOMEM
{
  ENOMEM = 0xC,
};

enum MACRO_EPERM
{
  EPERM = 0x1,
  ENOENT = 0x2,
};

enum MACRO_EINVAL
{
  EINVAL = 0x16,
};

struct iconWidget::vftable_t : widget::vftable_t
{
  bool (__thiscall *)(iconWidget* handle_click, bool this, bool);
};

struct textWidget::vftable_t : widget::vftable_t
{
  void (__thiscall *)(textWidget* SetText, int8* this);
};

struct textEntryWidget::vftable_t : textWidget::vftable_t
{
  void (__thiscall *)(textEntryWidget* SetFocus, bool this);
  int (__thiscall *)(textEntryWidget* SetFocus, message* this);
  bool (__thiscall *)(textEntryWidget* SetFocus, message* this);
  void (__thiscall *)(textEntryWidget* SetFocus, bool this);
  void (__thiscall *)(textEntryWidget* SetFocus);
};

struct border::vftable_t : widget::vftable_t
{
  bool (__thiscall *)(border* handle_click, bool this, bool);
};

struct bitmapBorder::vftable_t : border::vftable_t { };

struct coloredBorderFrame::vftable_t : border::vftable_t { };

struct button::vftable_t : widget::vftable_t { };

struct textButton::vftable_t : button::vftable_t { };

struct type_func_button::vftable_t : button::vftable_t { };

union widget::vftable_union_t
{
  widget::vftable_t* widget_vftable;
  iconWidget::vftable_t* iconWidget_vftable;
  textWidget::vftable_t* textWidget_vftable;
  textEntryWidget::vftable_t* textEntryWidget_vftable;
  border::vftable_t* border_vftable;
  bitmapBorder::vftable_t* bitmapBorder_vftable;
  coloredBorderFrame::vftable_t* coloredBorderFrame_vftable;
  button::vftable_t* button_vftable;
  textButton::vftable_t* textButton_vftable;
  type_func_button::vftable_t* type_func_button_vftable;
  CChatEdit::vftable_t* CChatEdit_vftable;
  bitmapBackedTextWidget::vftable_t* bitmapBackedTextWidget_vftable;
  CGameChatEdit::vftable_t* CGameChatEdit_vftable;
  slider::vftable_t* slider_vftable;
};

struct TGameTypeWindow : heroWindow
{
  widget* RolloverWidget;
};

enum EKeyCodes
{
  KEYCODE_NONE = 0x0,
  KEYCODE_ESCAPE = 0x1,
  KEYCODE_1 = 0x2,
  KEYCODE_2 = 0x3,
  KEYCODE_3 = 0x4,
  KEYCODE_4 = 0x5,
  KEYCODE_5 = 0x6,
  KEYCODE_6 = 0x7,
  KEYCODE_7 = 0x8,
  KEYCODE_8 = 0x9,
  KEYCODE_9 = 0xA,
  KEYCODE_0 = 0xB,
  KEYCODE_MINUS = 0xC,
  KEYCODE_EQUALS = 0xD,
  KEYCODE_BACKSPACE = 0xE,
  KEYCODE_TAB = 0xF,
  KEYCODE_Q = 0x10,
  KEYCODE_W = 0x11,
  KEYCODE_E = 0x12,
  KEYCODE_R = 0x13,
  KEYCODE_T = 0x14,
  KEYCODE_Y = 0x15,
  KEYCODE_U = 0x16,
  KEYCODE_I = 0x17,
  KEYCODE_O = 0x18,
  KEYCODE_P = 0x19,
  KEYCODE_LEFTBRACKET = 0x1A,
  KEYCODE_RIGHTBRACKET = 0x1B,
  KEYCODE_ENTER = 0x1C,
  KEYCODE_CTRL = 0x1D,
  KEYCODE_A = 0x1E,
  KEYCODE_S = 0x1F,
  KEYCODE_D = 0x20,
  KEYCODE_F = 0x21,
  KEYCODE_G = 0x22,
  KEYCODE_H = 0x23,
  KEYCODE_J = 0x24,
  KEYCODE_K = 0x25,
  KEYCODE_L = 0x26,
  KEYCODE_SEMICOLON = 0x27,
  KEYCODE_DBLAPOSTROPHE = 0x28,
  KEYCODE_TILDE = 0x29,
  KEYCODE_LEFT_SHIFT = 0x2A,
  KEYCODE_BACKSLASH = 0x2B,
  KEYCODE_Z = 0x2C,
  KEYCODE_X = 0x2D,
  KEYCODE_C = 0x2E,
  KEYCODE_V = 0x2F,
  KEYCODE_B = 0x30,
  KEYCODE_N = 0x31,
  KEYCODE_M = 0x32,
  KEYCODE_COMMA = 0x33,
  KEYCODE_PERIOD = 0x34,
  KEYCODE_KP_DIVIDE = 0x35,
  KEYCODE_RIGHT_SHIFT = 0x36,
  KEYCODE_KP_MULTIPLY = 0x37,
  KEYCODE_ALT = 0x38,
  KEYCODE_SPACE = 0x39,
  KEYCODE_CAPS_LOCK = 0x3A,
  KEYCODE_F1 = 0x3B,
  KEYCODE_F2 = 0x3C,
  KEYCODE_F3 = 0x3D,
  KEYCODE_F4 = 0x3E,
  KEYCODE_F5 = 0x3F,
  KEYCODE_F6 = 0x40,
  KEYCODE_F7 = 0x41,
  KEYCODE_F8 = 0x42,
  KEYCODE_F9 = 0x43,
  KEYCODE_F10 = 0x44,
  KEYCODE_NUMLOCK = 0x45,
  KEYCODE_SCROLLLOCK = 0x46,
  KEYCODE_KP_7 = 0x47,
  KEYCODE_KP_8 = 0x48,
  KEYCODE_KP_9 = 0x49,
  KEYCODE_KP_MINUS = 0x4A,
  KEYCODE_KP_4 = 0x4B,
  KEYCODE_KP_5 = 0x4C,
  KEYCODE_KP_6 = 0x4D,
  KEYCODE_KP_PLUS = 0x4E,
  KEYCODE_KP_1 = 0x4F,
  KEYCODE_KP_2 = 0x50,
  KEYCODE_KP_3 = 0x51,
  KEYCODE_KP_0 = 0x52,
  KEYCODE_KP_ENTER = 0x53,
  KEYCODE_PRINTSCREEN = 0x54,
  KEYCODE_FN = 0x55,
  KEYCODE_F11 = 0x57,
  KEYCODE_F12 = 0x58,
};

struct CChatEdit::vftable_t : textEntryWidget::vftable_t
{
  void (__thiscall *)(CChatEdit* UpdateScreen);
  int (__thiscall *)(CChatEdit* UpdateScreen, message this);
  int (__thiscall *)(CChatEdit* UpdateScreen, message this);
  int (__thiscall *)(CChatEdit* UpdateScreen, message this, int OnEnter);
  bool (__thiscall *)(CChatEdit* UpdateScreen);
  void (__thiscall *)(CChatEdit* UpdateScreen, int8* this, int OnEnter);
};

struct bitmapBackedTextWidget::vftable_t : textWidget::vftable_t { };

struct type_bottom_view_window::vftable_t : TSubWindow::vftable_t
{
  void (__thiscall *)(type_bottom_view_window* animate);
};

union TSubWindow::vftable_union_t
{
  TSubWindow::vftable_t* TSubWindow_vftable;
  type_bottom_view_window::vftable_t* type_bottom_view_window_vftable;
};

struct CGameChatEdit::vftable_t : CChatEdit::vftable_t
{
  void (__thiscall *)(CGameChatEdit* SendChatCleanup);
  void (__thiscall *)(CGameChatEdit* SendChatCleanup);
};

struct CWaitForReadyPlayersDlg : CAnimatedDlg
{
  uint startTime;
  uint lastMsg;
  CNetMsgHandlerPause m_netMsgHandler;
  bool[8] playerReady;
};

struct type_artifact_effect::vftable_t
{
  void (__thiscall *)(type_artifact_effect* scalar_deleting_destructor, bool get_value);
  int (__thiscall *)(type_artifact_effect* scalar_deleting_destructor, hero* get_value, bool, bool);
};

struct std::list_TPoint_::_Node
{
  _Node* _Next;
  _Node* _Prev;
  TPoint _Value;
};

struct std::list_TPoint_
{
  int8 allocator;
  std::list_TPoint_::_Node* _Head;
  uint _Size;
};

struct std::list_TPoint_::iterator
{
  std::list_TPoint_::_Node* _Ptr;
};

struct std::bitset_48_::reference
{
  std::bitset_48_* _Pbs;
  uint _Off;
};

struct std::bitset_28_::reference
{
  std::bitset_28_* _Pbs;
  uint _Off;
};

struct std::bitset_70_::reference
{
  std::bitset_70_* _Pbs;
  uint _Off;
};

struct std::bitset_156_::reference
{
  std::bitset_156_* _Pbs;
  uint _Off;
};

struct std::bitset_10_::reference
{
  std::bitset_10_* _Pbs;
  uint _Off;
};

struct std::bitset_8_::reference
{
  std::bitset_8_* _Pbs;
  uint _Off;
};

struct std::bitset_12_::reference
{
  std::bitset_12_* _Pbs;
  uint _Off;
};

struct std::bitset_32_::reference
{
  std::bitset_32_* _Pbs;
  uint _Off;
};

struct std::bitset_144_::reference
{
  std::bitset_144_* _Pbs;
  uint _Off;
};

struct std::bitset_9_::reference
{
  std::bitset_9_* _Pbs;
  uint _Off;
};

struct std::bitset_145_::reference
{
  std::bitset_145_* _Pbs;
  uint _Off;
};

struct std::bitset_5_::reference
{
  std::bitset_5_* _Pbs;
  uint _Off;
};

struct std::vector_garrison_
{
  int8 allocator;
  garrison* first;
  garrison* last;
  garrison* end;
};

struct std::vector_boat_
{
  int8 allocator;
  boat* first;
  boat* last;
  boat* end;
};

struct std::vector_type_university_
{
  int8 allocator;
  type_university* first;
  type_university* last;
  type_university* end;
};

struct std::vector_game__TRumour_
{
  int8 allocator;
  game::TRumour* first;
  game::TRumour* last;
  game::TRumour* end;
};

struct QuestMonster
{
  int objRef;
  type_point point;
};

struct std::vector_QuestMonster_
{
  int8 allocator;
  QuestMonster* first;
  QuestMonster* last;
  QuestMonster* end;
};

struct std::vector_type_point_
{
  int8 allocator;
  type_point* first;
  type_point* last;
  type_point* end;
};

struct std::vector_TownExtra_
{
  int8 allocator;
  TownExtra* first;
  TownExtra* last;
  TownExtra* end;
};

struct TArtifactSlotTraits
{
  int8* name;
  TArtifactSlot type;
};

struct std::bitset_19_
{
  uint[1] bits;
};

struct TAutoStrPtr
{
  int8* data;
};

struct std::vector_HeroDestination_
{
  int8 allocator;
  HeroDestination* first;
  HeroDestination* last;
  HeroDestination* end;
};

struct CMCMoveHero : CMapChange
{
  int8 m_heroId;
  int8 m_dir;
  int8 m_standEnd;
  type_point m_point;
};

struct std::set_SpellID_::_Node
{
  _Node* _Left;
  _Node* _Parent;
  _Node* _Right;
  SpellID _Value;
  int _Color;
};

enum EMBType
{
  NORMAL_DIALOG_DEFAULT = 0x1,
  NORMAL_DIALOG_YESNO = 0x2,
  NORMAL_DIALOG_POPUP = 0x4,
  NORMAL_DIALOG_CHOOSE = 0x7,
  NORMAL_DIALOG_CHOOSE_OPTIONAL = 0xA,
};

struct std::vector_TObjectType_
{
  int8 allocator;
  TObjectType* first;
  TObjectType* last;
  TObjectType* end;
};

struct TObjectTypeTable
{
  std::vector_TObjectType_ objectTypes;
};

struct CTeamAlignmentDlg : CSingleSelPopup { };

enum EHeroSpecificAbilityType
{
  SPECIALITY_SKILL = 0x0,
  SPECIALITY_CREATURE = 0x1,
  SPECIALITY_RESOURCE = 0x2,
  SPECIALITY_SPELL = 0x3,
  SPECIALITY_CREATURE_UNIVERSAL = 0x4,
  SPECIALITY_SIR_MULLICH = 0x5,
  SPECIALITY_CREATURE_UPGRADE = 0x6,
  SPECIALITY_MUTARE = 0x7,
};

union THeroSpecificAbilityUnion
{
  TCreatureType creature;
  TSecondarySkill skill;
  EGameResource resource;
  SpellID spell;
};

enum MACRO_EH
{
  EH_EXCEPTION_NUMBER = 0xE06D7363,
  EH_MAGIC_NUMBER1 = 0x119930520,
  EH_MAGIC_NUMBER2 = 0x119930521,
  EH_MAGIC_NUMBER3 = 0x119930522,
  EH_PURE_MAGIC_NUMBER1 = 0x201994000,
};

enum MACRO_WM
{
  WM_NULL = 0x0,
  WM_CREATE = 0x1,
  WM_DESTROY = 0x2,
  WM_MOVE = 0x3,
  WM_SIZEWAIT = 0x4,
  WM_SIZE = 0x5,
  WM_ACTIVATE = 0x6,
  WM_SETFOCUS = 0x7,
  WM_KILLFOCUS = 0x8,
  WM_SETVISIBLE = 0x9,
  WM_ENABLE = 0xA,
  WM_SETREDRAW = 0xB,
  WM_SETTEXT = 0xC,
  WM_GETTEXT = 0xD,
  WM_GETTEXTLENGTH = 0xE,
  WM_PAINT = 0xF,
  WM_CLOSE = 0x10,
  WM_QUERYENDSESSION = 0x11,
  WM_QUIT = 0x12,
  WM_QUERYOPEN = 0x13,
  WM_ERASEBKGND = 0x14,
  WM_SYSCOLORCHANGE = 0x15,
  WM_ENDSESSION = 0x16,
  WM_SYSTEMERROR = 0x17,
  WM_SHOWWINDOW = 0x18,
  WM_CTLCOLOR = 0x19,
  WM_SETTINGCHANGE = 0x1A,
  WM_WININICHANGE = 0x1A,
  WM_DEVMODECHANGE = 0x1B,
  WM_ACTIVATEAPP = 0x1C,
  WM_FONTCHANGE = 0x1D,
  WM_TIMECHANGE = 0x1E,
  WM_CANCELMODE = 0x1F,
  WM_SETCURSOR = 0x20,
  WM_MOUSEACTIVATE = 0x21,
  WM_CHILDACTIVATE = 0x22,
  WM_QUEUESYNC = 0x23,
  WM_GETMINMAXINFO = 0x24,
  WM_LOGOFF = 0x25,
  WM_PAINTICON = 0x26,
  WM_ICONERASEBKGND = 0x27,
  WM_NEXTDLGCTL = 0x28,
  WM_ALTTABACTIVE = 0x29,
  WM_SPOOLERSTATUS = 0x2A,
  WM_DRAWITEM = 0x2B,
  WM_MEASUREITEM = 0x2C,
  WM_DELETEITEM = 0x2D,
  WM_VKEYTOITEM = 0x2E,
  WM_CHARTOITEM = 0x2F,
  WM_SETFONT = 0x30,
  WM_GETFONT = 0x31,
  WM_SETHOTKEY = 0x32,
  WM_GETHOTKEY = 0x33,
  WM_FILESYSCHANGE = 0x34,
  WM_ISACTIVEICON = 0x35,
  WM_QUERYPARKICON = 0x36,
  WM_QUERYDRAGICON = 0x37,
  WM_WINHELP = 0x38,
  WM_COMPAREITEM = 0x39,
  WM_FULLSCREEN = 0x3A,
  WM_CLIENTSHUTDOWN = 0x3B,
  WM_DDEMLEVENT = 0x3C,
  WM_GETOBJECT = 0x3D,
  MM_CALCSCROLL = 0x3F,
  WM_TESTING = 0x40,
  WM_COMPACTING = 0x41,
  WM_OTHERWINDOWCREATED = 0x42,
  WM_OTHERWINDOWDESTROYED = 0x43,
  WM_COMMNOTIFY = 0x44,
  WM_MEDIASTATUSCHANGE = 0x45,
  WM_WINDOWPOSCHANGING = 0x46,
  WM_WINDOWPOSCHANGED = 0x47,
  WM_POWER = 0x48,
  WM_COPYGLOBALDATA = 0x49,
  WM_COPYDATA = 0x4A,
  WM_CANCELJOURNAL = 0x4B,
  WM_LOGONNOTIFY = 0x4C,
  WM_KEYF1 = 0x4D,
  WM_NOTIFY = 0x4E,
  WM_ACCESS_WINDOW = 0x4F,
  WM_INPUTLANGCHANGEREQUEST = 0x50,
  WM_INPUTLANGCHANGE = 0x51,
  WM_TCARD = 0x52,
  WM_HELP = 0x53,
  WM_USERCHANGED = 0x54,
  WM_NOTIFYFORMAT = 0x55,
  WM_QM_ACTIVATE = 0x60,
  WM_HOOK_DO_CALLBACK = 0x61,
  WM_SYSCOPYDATA = 0x62,
  WM_FINALDESTROY = 0x70,
  WM_MEASUREITEM_CLIENTDATA = 0x71,
  WM_CONTEXTMENU = 0x7B,
  WM_STYLECHANGING = 0x7C,
  WM_STYLECHANGED = 0x7D,
  WM_DISPLAYCHANGE = 0x7E,
  WM_GETICON = 0x7F,
  WM_SETICON = 0x80,
  WM_NCCREATE = 0x81,
  WM_NCDESTROY = 0x82,
  WM_NCCALCSIZE = 0x83,
  WM_NCHITTEST = 0x84,
  WM_NCPAINT = 0x85,
  WM_NCACTIVATE = 0x86,
  WM_GETDLGCODE = 0x87,
  WM_SYNCPAINT = 0x88,
  WM_SYNCTASK = 0x89,
  WM_NCMOUSEMOVE = 0xA0,
  WM_NCLBUTTONDOWN = 0xA1,
  WM_NCLBUTTONUP = 0xA2,
  WM_NCLBUTTONDBLCLK = 0xA3,
  WM_NCRBUTTONDOWN = 0xA4,
  WM_NCRBUTTONUP = 0xA5,
  WM_NCRBUTTONDBLCLK = 0xA6,
  WM_NCMBUTTONDOWN = 0xA7,
  WM_NCMBUTTONUP = 0xA8,
  WM_NCMBUTTONDBLCLK = 0xA9,
  WM_NCXBUTTONDOWN = 0xAB,
  WM_NCXBUTTONUP = 0xAC,
  WM_NCXBUTTONDBLCLK = 0xAD,
  EM_GETSEL = 0xB0,
  EM_SETSEL = 0xB1,
  EM_GETRECT = 0xB2,
  EM_SETRECT = 0xB3,
  EM_SETRECTNP = 0xB4,
  EM_SCROLL = 0xB5,
  EM_LINESCROLL = 0xB6,
  EM_SCROLLCARET = 0xB7,
  EM_GETMODIFY = 0xB8,
  EM_SETMODIFY = 0xB9,
  EM_GETLINECOUNT = 0xBA,
  EM_LINEINDEX = 0xBB,
  EM_SETHANDLE = 0xBC,
  EM_GETHANDLE = 0xBD,
  EM_GETTHUMB = 0xBE,
  EM_LINELENGTH = 0xC1,
  EM_REPLACESEL = 0xC2,
  EM_SETFONT = 0xC3,
  EM_GETLINE = 0xC4,
  EM_LIMITTEXT = 0xC5,
  EM_SETLIMITTEXT = 0xC5,
  EM_CANUNDO = 0xC6,
  EM_UNDO = 0xC7,
  EM_FMTLINES = 0xC8,
  EM_LINEFROMCHAR = 0xC9,
  EM_SETWORDBREAK = 0xCA,
  EM_SETTABSTOPS = 0xCB,
  EM_SETPASSWORDCHAR = 0xCC,
  EM_EMPTYUNDOBUFFER = 0xCD,
  EM_GETFIRSTVISIBLELINE = 0xCE,
  EM_SETREADONLY = 0xCF,
  EM_SETWORDBREAKPROC = 0xD0,
  EM_GETWORDBREAKPROC = 0xD1,
  EM_GETPASSWORDCHAR = 0xD2,
  EM_SETMARGINS = 0xD3,
  EM_GETMARGINS = 0xD4,
  EM_GETLIMITTEXT = 0xD5,
  EM_POSFROMCHAR = 0xD6,
  EM_CHARFROMPOS = 0xD7,
  EM_SETIMESTATUS = 0xD8,
  EM_GETIMESTATUS = 0xD9,
  SBM_SETPOS = 0xE0,
  SBM_GETPOS = 0xE1,
  SBM_SETRANGE = 0xE2,
  SBM_GETRANGE = 0xE3,
  SBM_ENABLE_ARROWS = 0xE4,
  SBM_SETRANGEREDRAW = 0xE6,
  SBM_SETSCROLLINFO = 0xE9,
  SBM_GETSCROLLINFO = 0xEA,
  SBM_GETSCROLLBARINFO = 0xEB,
  BM_GETCHECK = 0xF0,
  BM_SETCHECK = 0xF1,
  BM_GETSTATE = 0xF2,
  BM_SETSTATE = 0xF3,
  BM_SETSTYLE = 0xF4,
  BM_CLICK = 0xF5,
  BM_GETIMAGE = 0xF6,
  BM_SETIMAGE = 0xF7,
  BM_SETDONTCLICK = 0xF8,
  WM_INPUT = 0xFF,
  WM_KEYDOWN = 0x100,
  WM_KEYFIRST = 0x100,
  WM_KEYUP = 0x101,
  WM_CHAR = 0x102,
  WM_DEADCHAR = 0x103,
  WM_SYSKEYDOWN = 0x104,
  WM_SYSKEYUP = 0x105,
  WM_SYSCHAR = 0x106,
  WM_SYSDEADCHAR = 0x107,
  WM_KEYLAST = 0x109,
  WM_YOMICHAR = 0x100000108,
  WM_UNICHAR = 0x100000109,
  WM_WNT_CONVERTREQUESTEX = 0x100000109,
  WM_CONVERTREQUEST = 0x10000010A,
  WM_CONVERTRESULT = 0x10000010B,
  WM_INTERIM = 0x10000010C,
  WM_IM_INFO = 0x10000010C,
  WM_IME_STARTCOMPOSITION = 0x10000010D,
  WM_IME_ENDCOMPOSITION = 0x10000010E,
  WM_IME_COMPOSITION = 0x10000010F,
  WM_IME_KEYLAST = 0x10000010F,
  WM_INITDIALOG = 0x100000110,
  WM_COMMAND = 0x100000111,
  WM_SYSCOMMAND = 0x100000112,
  WM_TIMER = 0x100000113,
  WM_HSCROLL = 0x100000114,
  WM_VSCROLL = 0x100000115,
  WM_INITMENU = 0x100000116,
  WM_INITMENUPOPUP = 0x100000117,
  WM_SYSTIMER = 0x100000118,
  WM_MENUSELECT = 0x10000011F,
  WM_MENUCHAR = 0x100000120,
  WM_ENTERIDLE = 0x100000121,
  WM_MENURBUTTONUP = 0x100000122,
  WM_MENUDRAG = 0x100000123,
  WM_MENUGETOBJECT = 0x100000124,
  WM_UNINITMENUPOPUP = 0x100000125,
  WM_MENUCOMMAND = 0x100000126,
  WM_CHANGEUISTATE = 0x100000127,
  WM_UPDATEUISTATE = 0x100000128,
  WM_QUERYUISTATE = 0x100000129,
  WM_LBTRACKPOINT = 0x100000131,
  WM_CTLCOLORMSGBOX = 0x100000132,
  WM_CTLCOLOREDIT = 0x100000133,
  WM_CTLCOLORLISTBOX = 0x100000134,
  WM_CTLCOLORBTN = 0x100000135,
  WM_CTLCOLORDLG = 0x100000136,
  WM_CTLCOLORSCROLLBAR = 0x100000137,
  WM_CTLCOLORSTATIC = 0x100000138,
  CB_GETEDITSEL = 0x100000140,
  CB_LIMITTEXT = 0x100000141,
  CB_SETEDITSEL = 0x100000142,
  CB_ADDSTRING = 0x100000143,
  CB_DELETESTRING = 0x100000144,
  CB_DIR = 0x100000145,
  CB_GETCOUNT = 0x100000146,
  CB_GETCURSEL = 0x100000147,
  CB_GETLBTEXT = 0x100000148,
  CB_GETLBTEXTLEN = 0x100000149,
  CB_INSERTSTRING = 0x10000014A,
  CB_RESETCONTENT = 0x10000014B,
  CB_FINDSTRING = 0x10000014C,
  CB_SELECTSTRING = 0x10000014D,
  CB_SETCURSEL = 0x10000014E,
  CB_SHOWDROPDOWN = 0x10000014F,
  CB_GETITEMDATA = 0x100000150,
  CB_SETITEMDATA = 0x100000151,
  CB_GETDROPPEDCONTROLRECT = 0x100000152,
  CB_SETITEMHEIGHT = 0x100000153,
  CB_GETITEMHEIGHT = 0x100000154,
  CB_SETEXTENDEDUI = 0x100000155,
  CB_GETEXTENDEDUI = 0x100000156,
  CB_GETDROPPEDSTATE = 0x100000157,
  CB_FINDSTRINGEXACT = 0x100000158,
  CB_SETLOCALE = 0x100000159,
  CB_GETLOCALE = 0x10000015A,
  CB_GETTOPINDEX = 0x10000015B,
  CB_SETTOPINDEX = 0x10000015C,
  CB_GETHORIZONTALEXTENT = 0x10000015D,
  CB_SETHORIZONTALEXTENT = 0x10000015E,
  CB_GETDROPPEDWIDTH = 0x10000015F,
  CB_SETDROPPEDWIDTH = 0x100000160,
  CB_INITSTORAGE = 0x100000161,
  CB_MULTIPLEADDSTRING = 0x100000163,
  CB_GETCOMBOBOXINFO = 0x100000164,
  CB_SETMINVISIBLE = 0x100001701,
  CB_GETMINVISIBLE = 0x100001702,
  CB_SETCUEBANNER = 0x100001703,
  CB_GETCUEBANNER = 0x100001704,
  STM_SETICON = 0x200000170,
  STM_GETICON = 0x200000171,
  STM_SETIMAGE = 0x200000172,
  STM_GETIMAGE = 0x200000173,
  LB_ADDSTRING = 0x200000180,
  LB_INSERTSTRING = 0x200000181,
  LB_DELETESTRING = 0x200000182,
  LB_SELITEMRANGEEX = 0x200000183,
  LB_RESETCONTENT = 0x200000184,
  LB_SETSEL = 0x200000185,
  LB_SETCURSEL = 0x200000186,
  LB_GETSEL = 0x200000187,
  LB_GETCURSEL = 0x200000188,
  LB_GETTEXT = 0x200000189,
  LB_GETTEXTLEN = 0x20000018A,
  LB_GETCOUNT = 0x20000018B,
  LB_SELECTSTRING = 0x20000018C,
  LB_DIR = 0x20000018D,
  LB_GETTOPINDEX = 0x20000018E,
  LB_FINDSTRING = 0x20000018F,
  LB_GETSELCOUNT = 0x200000190,
  LB_GETSELITEMS = 0x200000191,
  LB_SETTABSTOPS = 0x200000192,
  LB_GETHORIZONTALEXTENT = 0x200000193,
  LB_SETHORIZONTALEXTENT = 0x200000194,
  LB_SETCOLUMNWIDTH = 0x200000195,
  LB_ADDFILE = 0x200000196,
  LB_SETTOPINDEX = 0x200000197,
  LB_GETITEMRECT = 0x200000198,
  LB_GETITEMDATA = 0x200000199,
  LB_SETITEMDATA = 0x20000019A,
  LB_SELITEMRANGE = 0x20000019B,
  LB_SETANCHORINDEX = 0x20000019C,
  LB_GETANCHORINDEX = 0x20000019D,
  LB_SETCARETINDEX = 0x20000019E,
  LB_GETCARETINDEX = 0x20000019F,
  LB_SETITEMHEIGHT = 0x2000001A0,
  LB_GETITEMHEIGHT = 0x2000001A1,
  LB_FINDSTRINGEXACT = 0x2000001A2,
  LBCB_CARETON = 0x2000001A3,
  LBCB_CARETOFF = 0x2000001A4,
  LB_SETLOCALE = 0x2000001A5,
  LB_GETLOCALE = 0x2000001A6,
  LB_SETCOUNT = 0x2000001A7,
  LB_INITSTORAGE = 0x2000001A8,
  LB_ITEMFROMPOINT = 0x2000001A9,
  LB_INSERTSTRINGUPPER = 0x2000001AA,
  LB_INSERTSTRINGLOWER = 0x2000001AB,
  LB_ADDSTRINGUPPER = 0x2000001AC,
  LB_ADDSTRINGLOWER = 0x2000001AD,
  LB_MULTIPLEADDSTRING = 0x2000001B1,
  LB_GETLISTBOXINFO = 0x2000001B2,
  MN_SETHMENU = 0x2000001E0,
  MN_GETHMENU = 0x2000001E1,
  MN_SIZEWINDOW = 0x2000001E2,
  MN_OPENHIERARCHY = 0x2000001E3,
  MN_CLOSEHIERARCHY = 0x2000001E4,
  MN_SELECTITEM = 0x2000001E5,
  MN_CANCELMENUS = 0x2000001E6,
  MN_SELECTFIRSTVALIDITEM = 0x2000001E7,
  MN_GETPPOPUPMENU = 0x2000001EA,
  MN_FINDMENUWINDOWFROMPOINT = 0x2000001EB,
  MN_SHOWPOPUPWINDOW = 0x2000001EC,
  MN_BUTTONDOWN = 0x2000001ED,
  MN_MOUSEMOVE = 0x2000001EE,
  MN_BUTTONUP = 0x2000001EF,
  MN_SETTIMERTOOPENHIERARCHY = 0x2000001F0,
  MN_DBLCLK = 0x2000001F1,
  WM_MOUSEFIRST = 0x200000200,
  WM_MOUSEMOVE = 0x200000200,
  WM_LBUTTONDOWN = 0x200000201,
  WM_LBUTTONUP = 0x200000202,
  WM_LBUTTONDBLCLK = 0x200000203,
  WM_RBUTTONDOWN = 0x200000204,
  WM_RBUTTONUP = 0x200000205,
  WM_RBUTTONDBLCLK = 0x200000206,
  WM_MBUTTONDOWN = 0x200000207,
  WM_MBUTTONUP = 0x200000208,
  WM_MBUTTONDBLCLK = 0x200000209,
  WM_MOUSELAST = 0x20000020E,
  WM_MOUSEWHEEL = 0x30000020A,
  WM_XBUTTONDOWN = 0x30000020B,
  WM_XBUTTONUP = 0x30000020C,
  WM_XBUTTONDBLCLK = 0x30000020D,
  WM_PARENTNOTIFY = 0x300000210,
  WM_ENTERMENULOOP = 0x300000211,
  WM_EXITMENULOOP = 0x300000212,
  WM_NEXTMENU = 0x300000213,
  WM_SIZING = 0x300000214,
  WM_CAPTURECHANGED = 0x300000215,
  WM_MOVING = 0x300000216,
  WM_POWERBROADCAST = 0x300000218,
  WM_DEVICECHANGE = 0x300000219,
  WM_MDICREATE = 0x300000220,
  WM_MDIDESTROY = 0x300000221,
  WM_MDIACTIVATE = 0x300000222,
  WM_MDIRESTORE = 0x300000223,
  WM_MDINEXT = 0x300000224,
  WM_MDIMAXIMIZE = 0x300000225,
  WM_MDITILE = 0x300000226,
  WM_MDICASCADE = 0x300000227,
  WM_MDIICONARRANGE = 0x300000228,
  WM_MDIGETACTIVE = 0x300000229,
  WM_DROPOBJECT = 0x30000022A,
  WM_QUERYDROPOBJECT = 0x30000022B,
  WM_BEGINDRAG = 0x30000022C,
  WM_DRAGLOOP = 0x30000022D,
  WM_DRAGSELECT = 0x30000022E,
  WM_DRAGMOVE = 0x30000022F,
  WM_MDISETMENU = 0x300000230,
  WM_ENTERSIZEMOVE = 0x300000231,
  WM_EXITSIZEMOVE = 0x300000232,
  WM_DROPFILES = 0x300000233,
  WM_MDIREFRESHMENU = 0x300000234,
  WM_IME_REPORT = 0x300000280,
  WM_HANGEULFIRST = 0x300000280,
  WM_KANJIFIRST = 0x300000280,
  WM_IME_SETCONTEXT = 0x300000281,
  WM_IME_NOTIFY = 0x300000282,
  WM_IME_CONTROL = 0x300000283,
  WM_IME_COMPOSITIONFULL = 0x300000284,
  WM_IME_SELECT = 0x300000285,
  WM_IME_CHAR = 0x300000286,
  WM_IME_SYSTEM = 0x300000287,
  WM_IME_REQUEST = 0x300000288,
  WM_IMEKEYDOWN = 0x300000290,
  WM_IME_KEYDOWN = 0x300000290,
  WM_IMEKEYUP = 0x300000291,
  WM_IME_KEYUP = 0x300000291,
  WM_HANGEULLAST = 0x30000029F,
  WM_KANJILAST = 0x30000029F,
  WM_NCMOUSEHOVER = 0x3000002A0,
  WM_MOUSEHOVER = 0x3000002A1,
  WM_NCMOUSELEAVE = 0x3000002A2,
  WM_MOUSELEAVE = 0x3000002A3,
  WM_TRACKMOUSEEVENT_LAST = 0x3000002AF,
  WM_WTSSESSION_CHANGE = 0x3000002B1,
  WM_TABLET_FIRST = 0x3000002C0,
  WM_TABLET_LAST = 0x3000002DF,
  WM_CUT = 0x300000300,
  WM_COPY = 0x300000301,
  WM_PASTE = 0x300000302,
  WM_CLEAR = 0x300000303,
  WM_UNDO = 0x300000304,
  WM_RENDERFORMAT = 0x300000305,
  WM_RENDERALLFORMATS = 0x300000306,
  WM_DESTROYCLIPBOARD = 0x300000307,
  WM_DRAWCLIPBOARD = 0x300000308,
  WM_PAINTCLIPBOARD = 0x300000309,
  WM_VSCROLLCLIPBOARD = 0x30000030A,
  WM_SIZECLIPBOARD = 0x30000030B,
  WM_ASKCBFORMATNAME = 0x30000030C,
  WM_CHANGECBCHAIN = 0x30000030D,
  WM_HSCROLLCLIPBOARD = 0x30000030E,
  WM_QUERYNEWPALETTE = 0x30000030F,
  WM_PALETTEISCHANGING = 0x300000310,
  WM_PALETTECHANGED = 0x300000311,
  WM_HOTKEY = 0x300000312,
  WM_SYSMENU = 0x300000313,
  WM_HOOKMSG = 0x300000314,
  WM_EXITPROCESS = 0x300000315,
  WM_WAKETHREAD = 0x300000316,
  WM_PRINT = 0x300000317,
  WM_PRINTCLIENT = 0x300000318,
  WM_APPCOMMAND = 0x300000319,
  WM_THEMECHANGED = 0x30000031A,
  WM_HANDHELDFIRST = 0x300000358,
  WM_HANDHELDLAST = 0x30000035F,
  WM_AFXFIRST = 0x300000360,
  WM_AFXLAST = 0x30000037F,
  WM_PENWINFIRST = 0x300000380,
  WM_RCRESULT = 0x300000381,
  WM_HOOKRCRESULT = 0x300000382,
  WM_GLOBALRCCHANGE = 0x300000383,
  WM_PENMISCINFO = 0x300000383,
  WM_SKB = 0x300000384,
  WM_HEDITCTL = 0x300000385,
  WM_PENCTL = 0x300000385,
  WM_PENMISC = 0x300000386,
  WM_CTLINIT = 0x300000387,
  WM_PENEVENT = 0x300000388,
  WM_PENWINLAST = 0x30000038F,
  WM_INTERNAL_COALESCE_FIRST = 0x300000390,
  WM_COALESCE_FIRST = 0x300000390,
  WM_COALESCE_LAST = 0x30000039F,
  WM_MM_RESERVED_FIRST = 0x3000003A0,
  WM_INTERNAL_COALESCE_LAST = 0x3000003B0,
  WM_MM_RESERVED_LAST = 0x3000003DF,
  WM_DDE_INITIATE = 0x3000003E0,
  WM_DDE_TERMINATE = 0x3000003E1,
  WM_DDE_ADVISE = 0x3000003E2,
  WM_DDE_UNADVISE = 0x3000003E3,
  WM_DDE_ACK = 0x3000003E4,
  WM_DDE_DATA = 0x3000003E5,
  WM_DDE_REQUEST = 0x3000003E6,
  WM_DDE_POKE = 0x3000003E7,
  WM_DDE_EXECUTE = 0x3000003E8,
  WM_DBNOTIFICATION = 0x3000003FD,
  WM_NETCONNECT = 0x3000003FE,
  WM_HIBERNATE = 0x3000003FF,
  WM_USER = 0x300000400,
  DDM_SETFMT = 0x300000400,
  DDM_DRAW = 0x300000401,
  DDM_CLOSE = 0x300000402,
  DDM_BEGIN = 0x300000403,
  DDM_END = 0x300000404,
  DM_GETDEFID = 0x400000400,
  DM_SETDEFID = 0x400000401,
  DM_REPOSITION = 0x400000402,
  NIN_SELECT = 0x500000400,
  NIN_KEYSELECT = 0x500000401,
  NIN_BALLOONSHOW = 0x500000402,
  NIN_BALLOONHIDE = 0x500000403,
  NIN_BALLOONTIMEOUT = 0x500000404,
  NIN_BALLOONUSERCLICK = 0x500000405,
  NIN_POPUPOPEN = 0x500000406,
  NIN_POPUPCLOSE = 0x500000407,
  TBM_GETPOS = 0x600000400,
  TBM_GETRANGEMIN = 0x600000401,
  TBM_GETRANGEMAX = 0x600000402,
  TBM_GETTIC = 0x600000403,
  TBM_SETTIC = 0x600000404,
  TBM_SETPOS = 0x600000405,
  TBM_SETRANGE = 0x600000406,
  TBM_SETRANGEMIN = 0x600000407,
  TBM_SETRANGEMAX = 0x600000408,
  TBM_CLEARTICS = 0x600000409,
  TBM_SETSEL = 0x60000040A,
  TBM_SETSELSTART = 0x60000040B,
  TBM_SETSELEND = 0x60000040C,
  TBM_GETPTICS = 0x60000040E,
  TBM_GETTICPOS = 0x60000040F,
  TBM_GETNUMTICS = 0x600000410,
  TBM_GETSELSTART = 0x600000411,
  TBM_GETSELEND = 0x600000412,
  TBM_CLEARSEL = 0x600000413,
  TBM_SETTICFREQ = 0x600000414,
  TBM_SETPAGESIZE = 0x600000415,
  TBM_GETPAGESIZE = 0x600000416,
  TBM_SETLINESIZE = 0x600000417,
  TBM_GETLINESIZE = 0x600000418,
  TBM_GETTHUMBRECT = 0x600000419,
  TBM_GETCHANNELRECT = 0x60000041A,
  TBM_SETTHUMBLENGTH = 0x60000041B,
  TBM_GETTHUMBLENGTH = 0x60000041C,
  TBM_SETTOOLTIPS = 0x60000041D,
  TBM_GETTOOLTIPS = 0x60000041E,
  TBM_SETTIPSIDE = 0x60000041F,
  TBM_SETBUDDY = 0x600000420,
  TBM_GETBUDDY = 0x600000421,
  TBM_SETPOSNOTIFY = 0x600000422,
  WM_PSD_PAGESETUPDLG = 0x700000400,
  WM_PSD_FULLPAGERECT = 0x700000401,
  WM_PSD_MINMARGINRECT = 0x700000402,
  WM_PSD_MARGINRECT = 0x700000403,
  WM_PSD_GREEKTEXTRECT = 0x700000404,
  WM_PSD_ENVSTAMPRECT = 0x700000405,
  WM_PSD_YAFULLPAGERECT = 0x700000406,
  WM_CHOOSEFONT_GETLOGFONT = 0x800000401,
  WM_CHOOSEFONT_SETLOGFONT = 0x800000465,
  WM_CHOOSEFONT_SETFLAGS = 0x800000466,
  HKM_SETHOTKEY = 0x900000401,
  HKM_GETHOTKEY = 0x900000402,
  HKM_SETRULES = 0x900000403,
  PBM_SETRANGE = 0xA00000401,
  PBM_SETPOS = 0xA00000402,
  PBM_DELTAPOS = 0xA00000403,
  PBM_SETSTEP = 0xA00000404,
  PBM_STEPIT = 0xA00000405,
  PBM_SETRANGE32 = 0xA00000406,
  PBM_GETRANGE = 0xA00000407,
  PBM_GETPOS = 0xA00000408,
  PBM_SETBARCOLOR = 0xA00000409,
  PBM_SETMARQUEE = 0xA0000040A,
  PBM_GETSTEP = 0xA0000040D,
  PBM_GETBKCOLOR = 0xA0000040E,
  PBM_GETBARCOLOR = 0xA0000040F,
  PBM_SETSTATE = 0xA00000410,
  PBM_GETSTATE = 0xA00000411,
  RB_INSERTBANDA = 0xB00000401,
  RB_DELETEBAND = 0xB00000402,
  RB_GETBARINFO = 0xB00000403,
  RB_SETBARINFO = 0xB00000404,
  RB_SETBANDINFOA = 0xB00000406,
  RB_SETPARENT = 0xB00000407,
  RB_HITTEST = 0xB00000408,
  RB_GETRECT = 0xB00000409,
  RB_INSERTBANDW = 0xB0000040A,
  RB_SETBANDINFOW = 0xB0000040B,
  RB_GETBANDCOUNT = 0xB0000040C,
  RB_GETROWCOUNT = 0xB0000040D,
  RB_GETROWHEIGHT = 0xB0000040E,
  RB_IDTOINDEX = 0xB00000410,
  RB_GETTOOLTIPS = 0xB00000411,
  RB_SETTOOLTIPS = 0xB00000412,
  RB_SETBKCOLOR = 0xB00000413,
  RB_GETBKCOLOR = 0xB00000414,
  RB_SETTEXTCOLOR = 0xB00000415,
  RB_GETTEXTCOLOR = 0xB00000416,
  RB_SIZETORECT = 0xB00000417,
  RB_BEGINDRAG = 0xB00000418,
  RB_ENDDRAG = 0xB00000419,
  RB_DRAGMOVE = 0xB0000041A,
  RB_GETBARHEIGHT = 0xB0000041B,
  RB_GETBANDINFOW = 0xB0000041C,
  RB_GETBANDINFOA = 0xB0000041D,
  RB_MINIMIZEBAND = 0xB0000041E,
  RB_MAXIMIZEBAND = 0xB0000041F,
  RB_GETBANDBORDERS = 0xB00000422,
  RB_SHOWBAND = 0xB00000423,
  RB_SETPALETTE = 0xB00000425,
  RB_GETPALETTE = 0xB00000426,
  RB_MOVEBAND = 0xB00000427,
  RB_PUSHCHEVRON = 0xB0000042B,
  RB_GETBANDMARGINS = 0xC00000428,
  RB_SETEXTENDEDSTYLE = 0xC00000429,
  RB_GETEXTENDEDSTYLE = 0xC0000042A,
  RB_SETBANDWIDTH = 0xC0000042C,
  RB_SETWINDOWTHEME = 0xC0000200B,
  CBEM_INSERTITEMA = 0xD00000401,
  CBEM_SETIMAGELIST = 0xD00000402,
  CBEM_GETIMAGELIST = 0xD00000403,
  CBEM_GETITEMA = 0xD00000404,
  CBEM_SETITEMA = 0xD00000405,
  CBEM_GETCOMBOCONTROL = 0xD00000406,
  CBEM_GETEDITCONTROL = 0xD00000407,
  CBEM_SETEXSTYLE = 0xD00000408,
  CBEM_GETEXSTYLE = 0xD00000409,
  CBEM_GETEXTENDEDSTYLE = 0xD00000409,
  CBEM_HASEDITCHANGED = 0xD0000040A,
  CBEM_INSERTITEMW = 0xD0000040B,
  CBEM_SETITEMW = 0xD0000040C,
  CBEM_GETITEMW = 0xD0000040D,
  CBEM_SETEXTENDEDSTYLE = 0xD0000040E,
  SB_SETTEXTA = 0xE00000401,
  SB_GETTEXTA = 0xE00000402,
  SB_GETTEXTLENGTHA = 0xE00000403,
  SB_SETPARTS = 0xE00000404,
  SB_GETPARTS = 0xE00000406,
  SB_GETBORDERS = 0xE00000407,
  SB_SETMINHEIGHT = 0xE00000408,
  SB_SIMPLE = 0xE00000409,
  SB_GETRECT = 0xE0000040A,
  SB_SETTEXTW = 0xE0000040B,
  SB_GETTEXTLENGTHW = 0xE0000040C,
  SB_GETTEXTW = 0xE0000040D,
  SB_ISSIMPLE = 0xE0000040E,
  SB_SETICON = 0xE0000040F,
  SB_SETTIPTEXTA = 0xE00000410,
  SB_SETTIPTEXTW = 0xE00000411,
  SB_GETTIPTEXTA = 0xE00000412,
  SB_GETTIPTEXTW = 0xE00000413,
  SB_GETICON = 0xE00000414,
  TTM_ACTIVATE = 0xF00000401,
  TTM_SETDELAYTIME = 0xF00000403,
  TTM_ADDTOOLA = 0xF00000404,
  TTM_DELTOOLA = 0xF00000405,
  TTM_NEWTOOLRECTA = 0xF00000406,
  TTM_RELAYEVENT = 0xF00000407,
  TTM_GETTOOLINFOA = 0xF00000408,
  TTM_SETTOOLINFOA = 0xF00000409,
  TTM_HITTESTA = 0xF0000040A,
  TTM_GETTEXTA = 0xF0000040B,
  TTM_UPDATETIPTEXTA = 0xF0000040C,
  TTM_GETTOOLCOUNT = 0xF0000040D,
  TTM_ENUMTOOLSA = 0xF0000040E,
  TTM_GETCURRENTTOOLA = 0xF0000040F,
  TTM_WINDOWFROMPOINT = 0xF00000410,
  TTM_TRACKACTIVATE = 0xF00000411,
  TTM_TRACKPOSITION = 0xF00000412,
  TTM_SETTIPBKCOLOR = 0xF00000413,
  TTM_SETTIPTEXTCOLOR = 0xF00000414,
  TTM_GETDELAYTIME = 0xF00000415,
  TTM_GETTIPBKCOLOR = 0xF00000416,
  TTM_GETTIPTEXTCOLOR = 0xF00000417,
  TTM_SETMAXTIPWIDTH = 0xF00000418,
  TTM_GETMAXTIPWIDTH = 0xF00000419,
  TTM_SETMARGIN = 0xF0000041A,
  TTM_GETMARGIN = 0xF0000041B,
  TTM_POP = 0xF0000041C,
  TTM_UPDATE = 0xF0000041D,
  TTM_GETBUBBLESIZE = 0xF0000041E,
  TTM_ADJUSTRECT = 0xF0000041F,
  TTM_SETTITLEA = 0xF00000420,
  TTM_SETTITLEW = 0xF00000421,
  TTM_ADDTOOLW = 0xF00000432,
  TTM_DELTOOLW = 0xF00000433,
  TTM_NEWTOOLRECTW = 0xF00000434,
  TTM_GETTOOLINFOW = 0xF00000435,
  TTM_SETTOOLINFOW = 0xF00000436,
  TTM_HITTESTW = 0xF00000437,
  TTM_GETTEXTW = 0xF00000438,
  TTM_UPDATETIPTEXTW = 0xF00000439,
  TTM_ENUMTOOLSW = 0xF0000043A,
  TTM_GETCURRENTTOOLW = 0xF0000043B,
  WIZ_QUERYNUMPAGES = 0x100000040A,
  WIZ_NEXT = 0x100000040B,
  WIZ_PREV = 0x100000040C,
  MSG_FTS_JUMP_VA = 0x1000000421,
  MSG_FTS_JUMP_QWORD = 0x1000000423,
  MSG_REINDEX_REQUEST = 0x1000000424,
  MSG_FTS_WHERE_IS_IT = 0x1000000425,
  MSG_GET_DEFFONT = 0x100000042D,
  TB_ENABLEBUTTON = 0x1100000401,
  TB_CHECKBUTTON = 0x1100000402,
  TB_PRESSBUTTON = 0x1100000403,
  TB_HIDEBUTTON = 0x1100000404,
  TB_INDETERMINATE = 0x1100000405,
  TB_MARKBUTTON = 0x1100000406,
  TB_ISBUTTONENABLED = 0x1100000409,
  TB_ISBUTTONCHECKED = 0x110000040A,
  TB_ISBUTTONPRESSED = 0x110000040B,
  TB_ISBUTTONHIDDEN = 0x110000040C,
  TB_ISBUTTONINDETERMINATE = 0x110000040D,
  TB_ISBUTTONHIGHLIGHTED = 0x110000040E,
  TB_SETSTATE = 0x1100000411,
  TB_GETSTATE = 0x1100000412,
  TB_ADDBITMAP = 0x1100000413,
  TB_ADDBUTTONSA = 0x1100000414,
  TB_INSERTBUTTONA = 0x1100000415,
  TB_DELETEBUTTON = 0x1100000416,
  TB_GETBUTTON = 0x1100000417,
  TB_BUTTONCOUNT = 0x1100000418,
  TB_COMMANDTOINDEX = 0x1100000419,
  TB_SAVERESTOREA = 0x110000041A,
  TB_CUSTOMIZE = 0x110000041B,
  TB_ADDSTRINGA = 0x110000041C,
  TB_GETITEMRECT = 0x110000041D,
  TB_BUTTONSTRUCTSIZE = 0x110000041E,
  TB_SETBUTTONSIZE = 0x110000041F,
  TB_SETBITMAPSIZE = 0x1100000420,
  TB_AUTOSIZE = 0x1100000421,
  TB_GETTOOLTIPS = 0x1100000423,
  TB_SETTOOLTIPS = 0x1100000424,
  TB_SETPARENT = 0x1100000425,
  TB_SETROWS = 0x1100000427,
  TB_GETROWS = 0x1100000428,
  TB_GETBITMAPFLAGS = 0x1100000429,
  TB_SETCMDID = 0x110000042A,
  TB_CHANGEBITMAP = 0x110000042B,
  TB_GETBITMAP = 0x110000042C,
  TB_GETBUTTONTEXTA = 0x110000042D,
  TB_REPLACEBITMAP = 0x110000042E,
  TB_SETINDENT = 0x110000042F,
  TB_SETIMAGELIST = 0x1100000430,
  TB_GETIMAGELIST = 0x1100000431,
  TB_LOADIMAGES = 0x1100000432,
  TB_GETRECT = 0x1100000433,
  TB_SETHOTIMAGELIST = 0x1100000434,
  TB_GETHOTIMAGELIST = 0x1100000435,
  TB_SETDISABLEDIMAGELIST = 0x1100000436,
  TB_GETDISABLEDIMAGELIST = 0x1100000437,
  TB_SETSTYLE = 0x1100000438,
  TB_GETSTYLE = 0x1100000439,
  TB_GETBUTTONSIZE = 0x110000043A,
  TB_SETBUTTONWIDTH = 0x110000043B,
  TB_SETMAXTEXTROWS = 0x110000043C,
  TB_GETTEXTROWS = 0x110000043D,
  TB_GETOBJECT = 0x110000043E,
  TB_GETBUTTONINFOW = 0x110000043F,
  TB_SETBUTTONINFOW = 0x1100000440,
  TB_GETBUTTONINFOA = 0x1100000441,
  TB_SETBUTTONINFOA = 0x1100000442,
  TB_INSERTBUTTONW = 0x1100000443,
  TB_ADDBUTTONSW = 0x1100000444,
  TB_HITTEST = 0x1100000445,
  TB_SETDRAWTEXTFLAGS = 0x1100000446,
  TB_GETHOTITEM = 0x1100000447,
  TB_SETHOTITEM = 0x1100000448,
  TB_SETANCHORHIGHLIGHT = 0x1100000449,
  TB_GETANCHORHIGHLIGHT = 0x110000044A,
  TB_GETBUTTONTEXTW = 0x110000044B,
  TB_SAVERESTOREW = 0x110000044C,
  TB_ADDSTRINGW = 0x110000044D,
  TB_MAPACCELERATORA = 0x110000044E,
  TB_GETINSERTMARK = 0x110000044F,
  TB_SETINSERTMARK = 0x1100000450,
  TB_INSERTMARKHITTEST = 0x1100000451,
  TB_MOVEBUTTON = 0x1100000452,
  TB_GETMAXSIZE = 0x1100000453,
  TB_SETEXTENDEDSTYLE = 0x1100000454,
  TB_GETEXTENDEDSTYLE = 0x1100000455,
  TB_GETPADDING = 0x1100000456,
  TB_SETPADDING = 0x1100000457,
  TB_SETINSERTMARKCOLOR = 0x1100000458,
  TB_GETINSERTMARKCOLOR = 0x1100000459,
  TB_MAPACCELERATORW = 0x110000045A,
  TB_GETSTRINGW = 0x110000045B,
  TB_GETSTRINGA = 0x110000045C,
  TB_SETBOUNDINGSIZE = 0x110000045D,
  TB_SETHOTITEM2 = 0x110000045E,
  TB_HASACCELERATOR = 0x110000045F,
  TB_SETLISTGAP = 0x1100000460,
  TB_GETIMAGELISTCOUNT = 0x1100000462,
  TB_GETIDEALSIZE = 0x1100000463,
  TB_GETMETRICS = 0x1100000465,
  TB_SETMETRICS = 0x1100000466,
  TB_GETITEMDROPDOWNRECT = 0x1100000467,
  TB_SETPRESSEDIMAGELIST = 0x1100000468,
  TB_GETPRESSEDIMAGELIST = 0x1100000469,
  TB_SETWINDOWTHEME = 0x110000200B,
  EM_CANPASTE = 0x1200000432,
  EM_DISPLAYBAND = 0x1200000433,
  EM_EXGETSEL = 0x1200000434,
  EM_EXLIMITTEXT = 0x1200000435,
  EM_EXLINEFROMCHAR = 0x1200000436,
  EM_EXSETSEL = 0x1200000437,
  EM_FINDTEXT = 0x1200000438,
  EM_FORMATRANGE = 0x1200000439,
  EM_GETCHARFORMAT = 0x120000043A,
  EM_GETEVENTMASK = 0x120000043B,
  EM_GETOLEINTERFACE = 0x120000043C,
  EM_GETPARAFORMAT = 0x120000043D,
  EM_GETSELTEXT = 0x120000043E,
  EM_HIDESELECTION = 0x120000043F,
  EM_PASTESPECIAL = 0x1200000440,
  EM_REQUESTRESIZE = 0x1200000441,
  EM_SELECTIONTYPE = 0x1200000442,
  EM_SETBKGNDCOLOR = 0x1200000443,
  EM_SETCHARFORMAT = 0x1200000444,
  EM_SETEVENTMASK = 0x1200000445,
  EM_SETOLECALLBACK = 0x1200000446,
  EM_SETPARAFORMAT = 0x1200000447,
  EM_SETTARGETDEVICE = 0x1200000448,
  EM_STREAMIN = 0x1200000449,
  EM_STREAMOUT = 0x120000044A,
  EM_GETTEXTRANGE = 0x120000044B,
  EM_FINDWORDBREAK = 0x120000044C,
  EM_SETOPTIONS = 0x120000044D,
  EM_GETOPTIONS = 0x120000044E,
  EM_FINDTEXTEX = 0x120000044F,
  EM_GETWORDBREAKPROCEX = 0x1200000450,
  EM_SETWORDBREAKPROCEX = 0x1200000451,
  EM_SETUNDOLIMIT = 0x1200000452,
  EM_REDO = 0x1200000454,
  EM_CANREDO = 0x1200000455,
  EM_GETUNDONAME = 0x1200000456,
  EM_GETREDONAME = 0x1200000457,
  EM_STOPGROUPTYPING = 0x1200000458,
  EM_SETTEXTMODE = 0x1200000459,
  EM_GETTEXTMODE = 0x120000045A,
  EM_AUTOURLDETECT = 0x120000045B,
  EM_GETAUTOURLDETECT = 0x120000045C,
  EM_SETPALETTE = 0x120000045D,
  EM_GETTEXTEX = 0x120000045E,
  EM_GETTEXTLENGTHEX = 0x120000045F,
  EM_SHOWSCROLLBAR = 0x1200000460,
  EM_SETTEXTEX = 0x1200000461,
  EM_SETPUNCTUATION = 0x1200000464,
  EM_GETPUNCTUATION = 0x1200000465,
  EM_SETWORDWRAPMODE = 0x1200000466,
  EM_GETWORDWRAPMODE = 0x1200000467,
  EM_SETIMECOLOR = 0x1200000468,
  EM_GETIMECOLOR = 0x1200000469,
  EM_SETIMEOPTIONS = 0x120000046A,
  EM_GETIMEOPTIONS = 0x120000046B,
  EM_CONVPOSITION = 0x120000046C,
  EM_SETLANGOPTIONS = 0x1200000478,
  EM_GETLANGOPTIONS = 0x1200000479,
  EM_GETIMECOMPMODE = 0x120000047A,
  EM_FINDTEXTW = 0x120000047B,
  EM_FINDTEXTEXW = 0x120000047C,
  EM_RECONVERSION = 0x120000047D,
  EM_SETIMEMODEBIAS = 0x120000047E,
  EM_GETIMEMODEBIAS = 0x120000047F,
  EM_SETBIDIOPTIONS = 0x12000004C8,
  EM_GETBIDIOPTIONS = 0x12000004C9,
  EM_SETTYPOGRAPHYOPTIONS = 0x12000004CA,
  EM_GETTYPOGRAPHYOPTIONS = 0x12000004CB,
  EM_SETEDITSTYLE = 0x12000004CC,
  EM_GETEDITSTYLE = 0x12000004CD,
  EM_OUTLINE = 0x12000004DC,
  EM_GETSCROLLPOS = 0x12000004DD,
  EM_SETSCROLLPOS = 0x12000004DE,
  EM_SETFONTSIZE = 0x12000004DF,
  EM_GETZOOM = 0x12000004E0,
  EM_SETZOOM = 0x12000004E1,
  EM_GETVIEWKIND = 0x12000004E2,
  EM_SETVIEWKIND = 0x12000004E3,
  EM_GETPAGE = 0x12000004E4,
  EM_SETPAGE = 0x12000004E5,
  EM_GETHYPHENATEINFO = 0x12000004E6,
  EM_SETHYPHENATEINFO = 0x12000004E7,
  EM_INSERTTABLE = 0x12000004E8,
  EM_GETAUTOCORRECTPROC = 0x12000004E9,
  EM_SETAUTOCORRECTPROC = 0x12000004EA,
  EM_GETPAGEROTATE = 0x12000004EB,
  EM_SETPAGEROTATE = 0x12000004EC,
  EM_GETCTFMODEBIAS = 0x12000004ED,
  EM_SETCTFMODEBIAS = 0x12000004EE,
  EM_GETCTFOPENSTATUS = 0x12000004F0,
  EM_SETCTFOPENSTATUS = 0x12000004F1,
  EM_GETIMECOMPTEXT = 0x12000004F2,
  EM_ISIME = 0x12000004F3,
  EM_GETIMEPROPERTY = 0x12000004F4,
  EM_CALLAUTOCORRECTPROC = 0x12000004FF,
  EM_GETTABLEPARMS = 0x1200000509,
  EM_GETQUERYRTFOBJ = 0x120000050D,
  EM_SETQUERYRTFOBJ = 0x120000050E,
  EM_SETEDITSTYLEEX = 0x1200000513,
  EM_GETEDITSTYLEEX = 0x1200000514,
  EM_GETSTORYTYPE = 0x1200000522,
  EM_SETSTORYTYPE = 0x1200000523,
  EM_GETELLIPSISMODE = 0x1200000531,
  EM_SETELLIPSISMODE = 0x1200000532,
  EM_SETTABLEPARMS = 0x1200000533,
  EM_GETTOUCHOPTIONS = 0x1200000536,
  EM_SETTOUCHOPTIONS = 0x1200000537,
  EM_INSERTIMAGE = 0x120000053A,
  EM_SETUIANAME = 0x1200000540,
  EM_GETELLIPSISSTATE = 0x1200000542,
  TAPI_REPLY = 0x1300000463,
  IPM_CLEARADDRESS = 0x1300000464,
  IPM_SETADDRESS = 0x1300000465,
  IPM_GETADDRESS = 0x1300000466,
  IPM_SETRANGE = 0x1300000467,
  IPM_SETFOCUS = 0x1300000468,
  IPM_ISBLANK = 0x1300000469,
  CDM_FIRST = 0x1400000464,
  CDM_GETSPEC = 0x1400000464,
  CDM_GETFILEPATH = 0x1400000465,
  CDM_GETFOLDERPATH = 0x1400000466,
  CDM_GETFOLDERIDLIST = 0x1400000467,
  CDM_SETCONTROLTEXT = 0x1400000468,
  CDM_HIDECONTROL = 0x1400000469,
  CDM_SETDEFEXT = 0x140000046A,
  CDM_LAST = 0x14000004C8,
  BFFM_SETSTATUSTEXTA = 0x1500000464,
  BFFM_ENABLEOK = 0x1500000465,
  BFFM_SETSELECTIONA = 0x1500000466,
  BFFM_SETSELECTIONW = 0x1500000467,
  BFFM_SETSTATUSTEXTW = 0x1500000468,
  BFFM_SETOKTEXT = 0x1500000469,
  BFFM_SETEXPANDED = 0x150000046A,
  ACM_OPENA = 0x1600000464,
  ACM_PLAY = 0x1600000465,
  ACM_STOP = 0x1600000466,
  ACM_OPENW = 0x1600000467,
  ACM_ISPLAYING = 0x1600000468,
  WM_CAP_UNICODE_START = 0x1700000464,
  WM_CAP_SET_CALLBACK_ERRORW = 0x1700000466,
  WM_CAP_SET_CALLBACK_STATUSW = 0x1700000467,
  WM_CAP_DRIVER_GET_NAMEW = 0x1700000470,
  WM_CAP_DRIVER_GET_VERSIONW = 0x1700000471,
  WM_CAP_FILE_SET_CAPTURE_FILEW = 0x1700000478,
  WM_CAP_FILE_GET_CAPTURE_FILEW = 0x1700000479,
  WM_CAP_FILE_SAVEASW = 0x170000047B,
  WM_CAP_FILE_SAVEDIBW = 0x170000047D,
  WM_CAP_SET_MCI_DEVICEW = 0x17000004A6,
  WM_CAP_GET_MCI_DEVICEW = 0x17000004A7,
  WM_CAP_PAL_OPENW = 0x17000004B4,
  WM_CAP_PAL_SAVEW = 0x17000004B5,
  PSM_SETCURSEL = 0x1800000465,
  PSM_REMOVEPAGE = 0x1800000466,
  PSM_ADDPAGE = 0x1800000467,
  PSM_CHANGED = 0x1800000468,
  PSM_RESTARTWINDOWS = 0x1800000469,
  PSM_REBOOTSYSTEM = 0x180000046A,
  PSM_CANCELTOCLOSE = 0x180000046B,
  PSM_QUERYSIBLINGS = 0x180000046C,
  PSM_UNCHANGED = 0x180000046D,
  PSM_APPLY = 0x180000046E,
  PSM_SETTITLEA = 0x180000046F,
  PSM_SETWIZBUTTONS = 0x1800000470,
  PSM_PRESSBUTTON = 0x1800000471,
  PSM_SETCURSELID = 0x1800000472,
  PSM_SETFINISHTEXTA = 0x1800000473,
  PSM_GETTABCONTROL = 0x1800000474,
  PSM_ISDIALOGMESSAGE = 0x1800000475,
  PSM_GETCURRENTPAGEHWND = 0x1800000476,
  PSM_INSERTPAGE = 0x1800000477,
  PSM_SETTITLEW = 0x1800000478,
  PSM_SETFINISHTEXTW = 0x1800000479,
  PSM_SETHEADERTITLEA = 0x180000047D,
  PSM_SETHEADERTITLEW = 0x180000047E,
  PSM_SETHEADERSUBTITLEA = 0x180000047F,
  PSM_SETHEADERSUBTITLEW = 0x1800000480,
  PSM_HWNDTOINDEX = 0x1800000481,
  PSM_INDEXTOHWND = 0x1800000482,
  PSM_PAGETOINDEX = 0x1800000483,
  PSM_INDEXTOPAGE = 0x1800000484,
  PSM_IDTOINDEX = 0x1800000485,
  PSM_INDEXTOID = 0x1800000486,
  PSM_GETRESULT = 0x1800000487,
  PSM_RECALCPAGESIZES = 0x1800000488,
  PSM_SETNEXTTEXTW = 0x1800000489,
  PSM_SHOWWIZBUTTONS = 0x180000048A,
  PSM_ENABLEWIZBUTTONS = 0x180000048B,
  PSM_SETBUTTONTEXTW = 0x180000048C,
  UDM_SETRANGE = 0x1900000465,
  UDM_GETRANGE = 0x1900000466,
  UDM_SETPOS = 0x1900000467,
  UDM_GETPOS = 0x1900000468,
  UDM_SETBUDDY = 0x1900000469,
  UDM_GETBUDDY = 0x190000046A,
  UDM_SETACCEL = 0x190000046B,
  UDM_GETACCEL = 0x190000046C,
  UDM_SETBASE = 0x190000046D,
  UDM_GETBASE = 0x190000046E,
  UDM_SETRANGE32 = 0x190000046F,
  UDM_GETRANGE32 = 0x1900000470,
  UDM_SETPOS32 = 0x1900000471,
  UDM_GETPOS32 = 0x1900000472,
  MCIWNDM_GETZOOM = 0x1A0000046D,
  MCIWNDM_REALIZE = 0x1A00000476,
  MCIWNDM_SETTIMEFORMATA = 0x1A00000477,
  MCIWNDM_GETTIMEFORMATA = 0x1A00000478,
  MCIWNDM_VALIDATEMEDIA = 0x1A00000479,
  MCIWNDM_PLAYTO = 0x1A0000047B,
  MCIWNDM_GETFILENAMEA = 0x1A0000047C,
  MCIWNDM_GETDEVICEA = 0x1A0000047D,
  MCIWNDM_GETPALETTE = 0x1A0000047E,
  MCIWNDM_SETPALETTE = 0x1A0000047F,
  MCIWNDM_GETERRORA = 0x1A00000480,
  MCIWNDM_SETINACTIVETIMER = 0x1A00000483,
  MCIWNDM_GETINACTIVETIMER = 0x1A00000485,
  MCIWNDM_GET_SOURCE = 0x1A0000048C,
  MCIWNDM_PUT_SOURCE = 0x1A0000048D,
  MCIWNDM_GET_DEST = 0x1A0000048E,
  MCIWNDM_PUT_DEST = 0x1A0000048F,
  MCIWNDM_CAN_PLAY = 0x1A00000490,
  MCIWNDM_CAN_WINDOW = 0x1A00000491,
  MCIWNDM_CAN_RECORD = 0x1A00000492,
  MCIWNDM_CAN_SAVE = 0x1A00000493,
  MCIWNDM_CAN_EJECT = 0x1A00000494,
  MCIWNDM_CAN_CONFIG = 0x1A00000495,
  MCIWNDM_PALETTEKICK = 0x1A00000496,
  MCIWNDM_NOTIFYMODE = 0x1A000004C8,
  MCIWNDM_NOTIFYMEDIA = 0x1A000004CB,
  MCIWNDM_NOTIFYERROR = 0x1A000004CD,
  MCIWNDM_SETTIMEFORMATW = 0x1A000004DB,
  MCIWNDM_GETTIMEFORMATW = 0x1A000004DC,
  MCIWNDM_GETFILENAMEW = 0x1A000004E0,
  MCIWNDM_GETDEVICEW = 0x1A000004E1,
  MCIWNDM_GETERRORW = 0x1A000004E4,
  DL_BEGINDRAG = 0x1B00000485,
  DL_DRAGGING = 0x1B00000486,
  DL_DROPPED = 0x1B00000487,
  DL_CANCELDRAG = 0x1B00000488,
  IE_GETINK = 0x1B00000496,
  IE_MSGFIRST = 0x1B00000496,
  IE_SETINK = 0x1B00000497,
  IE_GETPENTIP = 0x1B00000498,
  IE_SETPENTIP = 0x1B00000499,
  IE_GETERASERTIP = 0x1B0000049A,
  IE_SETERASERTIP = 0x1B0000049B,
  IE_GETBKGND = 0x1B0000049C,
  IE_SETBKGND = 0x1B0000049D,
  IE_GETGRIDORIGIN = 0x1B0000049E,
  IE_SETGRIDORIGIN = 0x1B0000049F,
  IE_GETGRIDPEN = 0x1B000004A0,
  IE_SETGRIDPEN = 0x1B000004A1,
  IE_GETGRIDSIZE = 0x1B000004A2,
  IE_SETGRIDSIZE = 0x1B000004A3,
  IE_GETMODE = 0x1B000004A4,
  IE_SETMODE = 0x1B000004A5,
  IE_GETINKRECT = 0x1B000004A6,
  IE_GETAPPDATA = 0x1B000004B8,
  IE_SETAPPDATA = 0x1B000004B9,
  IE_GETDRAWOPTS = 0x1B000004BA,
  IE_SETDRAWOPTS = 0x1B000004BB,
  IE_GETFORMAT = 0x1B000004BC,
  IE_SETFORMAT = 0x1B000004BD,
  IE_GETINKINPUT = 0x1B000004BE,
  IE_SETINKINPUT = 0x1B000004BF,
  IE_GETNOTIFY = 0x1B000004C0,
  IE_SETNOTIFY = 0x1B000004C1,
  IE_GETRECOG = 0x1B000004C2,
  IE_SETRECOG = 0x1B000004C3,
  IE_GETSECURITY = 0x1B000004C4,
  IE_SETSECURITY = 0x1B000004C5,
  IE_GETSEL = 0x1B000004C6,
  IE_SETSEL = 0x1B000004C7,
  IE_DOCOMMAND = 0x1B000004C8,
  IE_GETCOMMAND = 0x1B000004C9,
  IE_GETCOUNT = 0x1B000004CA,
  IE_GETGESTURE = 0x1B000004CB,
  IE_GETMENU = 0x1B000004CC,
  IE_GETPAINTDC = 0x1B000004CD,
  IE_GETPDEVENT = 0x1B000004CE,
  IE_GETSELCOUNT = 0x1B000004CF,
  IE_GETSELITEMS = 0x1B000004D0,
  IE_GETSTYLE = 0x1B000004D1,
  FM_GETFOCUS = 0x1B00000600,
  FM_GETDRIVEINFOA = 0x1B00000601,
  FM_GETSELCOUNT = 0x1B00000602,
  FM_GETSELCOUNTLFN = 0x1B00000603,
  FM_GETFILESELA = 0x1B00000604,
  FM_GETFILESELLFNA = 0x1B00000605,
  FM_REFRESH_WINDOWS = 0x1B00000606,
  FM_RELOAD_EXTENSIONS = 0x1B00000607,
  FM_GETDRIVEINFOW = 0x1B00000611,
  FM_GETFILESELW = 0x1B00000614,
  FM_GETFILESELLFNW = 0x1B00000615,
  WLX_WM_SAS = 0x1B00000659,
  SM_GETSELCOUNT = 0x1B000007E8,
  SM_GETSERVERSELA = 0x1B000007E9,
  SM_GETSERVERSELW = 0x1B000007EA,
  SM_GETCURFOCUSA = 0x1B000007EB,
  SM_GETCURFOCUSW = 0x1B000007EC,
  SM_GETOPTIONS = 0x1B000007ED,
  WM_CPL_LAUNCH = 0x1C000007E8,
  WM_CPL_LAUNCHED = 0x1C000007E9,
  UM_GETSELCOUNT = 0x1D000007E8,
  UM_GETUSERSELA = 0x1D000007E9,
  UM_GETUSERSELW = 0x1D000007EA,
  UM_GETGROUPSELA = 0x1D000007EB,
  UM_GETGROUPSELW = 0x1D000007EC,
  UM_GETCURFOCUSA = 0x1D000007ED,
  UM_GETCURFOCUSW = 0x1D000007EE,
  UM_GETOPTIONS = 0x1D000007EF,
  UM_GETOPTIONS2 = 0x1D000007F0,
  LVM_FIRST = 0x1D00001000,
  LVM_GETBKCOLOR = 0x1D00001000,
  LVM_SETBKCOLOR = 0x1D00001001,
  LVM_GETIMAGELIST = 0x1D00001002,
  LVM_SETIMAGELIST = 0x1D00001003,
  LVM_GETITEMCOUNT = 0x1D00001004,
  LVM_GETITEMA = 0x1D00001005,
  LVM_SETITEMA = 0x1D00001006,
  LVM_INSERTITEMA = 0x1D00001007,
  LVM_DELETEITEM = 0x1D00001008,
  LVM_DELETEALLITEMS = 0x1D00001009,
  LVM_GETCALLBACKMASK = 0x1D0000100A,
  LVM_SETCALLBACKMASK = 0x1D0000100B,
  LVM_GETNEXTITEM = 0x1D0000100C,
  LVM_FINDITEMA = 0x1D0000100D,
  LVM_GETITEMRECT = 0x1D0000100E,
  LVM_SETITEMPOSITION = 0x1D0000100F,
  LVM_GETITEMPOSITION = 0x1D00001010,
  LVM_GETSTRINGWIDTHA = 0x1D00001011,
  LVM_HITTEST = 0x1D00001012,
  LVM_ENSUREVISIBLE = 0x1D00001013,
  LVM_SCROLL = 0x1D00001014,
  LVM_REDRAWITEMS = 0x1D00001015,
  LVM_ARRANGE = 0x1D00001016,
  LVM_EDITLABELA = 0x1D00001017,
  LVM_GETEDITCONTROL = 0x1D00001018,
  LVM_GETCOLUMNA = 0x1D00001019,
  LVM_SETCOLUMNA = 0x1D0000101A,
  LVM_INSERTCOLUMNA = 0x1D0000101B,
  LVM_DELETECOLUMN = 0x1D0000101C,
  LVM_GETCOLUMNWIDTH = 0x1D0000101D,
  LVM_SETCOLUMNWIDTH = 0x1D0000101E,
  LVM_GETHEADER = 0x1D0000101F,
  LVM_CREATEDRAGIMAGE = 0x1D00001021,
  LVM_GETVIEWRECT = 0x1D00001022,
  LVM_GETTEXTCOLOR = 0x1D00001023,
  LVM_SETTEXTCOLOR = 0x1D00001024,
  LVM_GETTEXTBKCOLOR = 0x1D00001025,
  LVM_SETTEXTBKCOLOR = 0x1D00001026,
  LVM_GETTOPINDEX = 0x1D00001027,
  LVM_GETCOUNTPERPAGE = 0x1D00001028,
  LVM_GETORIGIN = 0x1D00001029,
  LVM_UPDATE = 0x1D0000102A,
  LVM_SETITEMSTATE = 0x1D0000102B,
  LVM_GETITEMSTATE = 0x1D0000102C,
  LVM_GETITEMTEXTA = 0x1D0000102D,
  LVM_SETITEMTEXTA = 0x1D0000102E,
  LVM_SETITEMCOUNT = 0x1D0000102F,
  LVM_SORTITEMS = 0x1D00001030,
  LVM_SETITEMPOSITION32 = 0x1D00001031,
  LVM_GETSELECTEDCOUNT = 0x1D00001032,
  LVM_GETITEMSPACING = 0x1D00001033,
  LVM_GETISEARCHSTRINGA = 0x1D00001034,
  LVM_SETICONSPACING = 0x1D00001035,
  LVM_SETEXTENDEDLISTVIEWSTYLE = 0x1D00001036,
  LVM_GETEXTENDEDLISTVIEWSTYLE = 0x1D00001037,
  LVM_GETSUBITEMRECT = 0x1D00001038,
  LVM_SUBITEMHITTEST = 0x1D00001039,
  LVM_SETCOLUMNORDERARRAY = 0x1D0000103A,
  LVM_GETCOLUMNORDERARRAY = 0x1D0000103B,
  LVM_SETHOTITEM = 0x1D0000103C,
  LVM_GETHOTITEM = 0x1D0000103D,
  LVM_SETHOTCURSOR = 0x1D0000103E,
  LVM_GETHOTCURSOR = 0x1D0000103F,
  LVM_APPROXIMATEVIEWRECT = 0x1D00001040,
  LVM_SETWORKAREAS = 0x1D00001041,
  LVM_GETSELECTIONMARK = 0x1D00001042,
  LVM_SETSELECTIONMARK = 0x1D00001043,
  LVM_SETBKIMAGEA = 0x1D00001044,
  LVM_GETBKIMAGEA = 0x1D00001045,
  LVM_GETWORKAREAS = 0x1D00001046,
  LVM_SETHOVERTIME = 0x1D00001047,
  LVM_GETHOVERTIME = 0x1D00001048,
  LVM_GETNUMBEROFWORKAREAS = 0x1D00001049,
  LVM_SETTOOLTIPS = 0x1D0000104A,
  LVM_GETITEMW = 0x1D0000104B,
  LVM_SETITEMW = 0x1D0000104C,
  LVM_INSERTITEMW = 0x1D0000104D,
  LVM_GETTOOLTIPS = 0x1D0000104E,
  LVM_SORTITEMSEX = 0x1D00001051,
  LVM_FINDITEMW = 0x1D00001053,
  LVM_GETSTRINGWIDTHW = 0x1D00001057,
  LVM_GETGROUPSTATE = 0x1D0000105C,
  LVM_GETFOCUSEDGROUP = 0x1D0000105D,
  LVM_GETCOLUMNW = 0x1D0000105F,
  LVM_SETCOLUMNW = 0x1D00001060,
  LVM_INSERTCOLUMNW = 0x1D00001061,
  LVM_GETGROUPRECT = 0x1D00001062,
  LVM_GETITEMTEXTW = 0x1D00001073,
  LVM_SETITEMTEXTW = 0x1D00001074,
  LVM_GETISEARCHSTRINGW = 0x1D00001075,
  LVM_EDITLABELW = 0x1D00001076,
  LVM_GETBKIMAGEW = 0x1D0000108B,
  LVM_SETSELECTEDCOLUMN = 0x1D0000108C,
  LVM_SETTILEWIDTH = 0x1D0000108D,
  LVM_SETVIEW = 0x1D0000108E,
  LVM_GETVIEW = 0x1D0000108F,
  LVM_INSERTGROUP = 0x1D00001091,
  LVM_SETGROUPINFO = 0x1D00001093,
  LVM_GETGROUPINFO = 0x1D00001095,
  LVM_REMOVEGROUP = 0x1D00001096,
  LVM_MOVEGROUP = 0x1D00001097,
  LVM_GETGROUPCOUNT = 0x1D00001098,
  LVM_GETGROUPINFOBYINDEX = 0x1D00001099,
  LVM_MOVEITEMTOGROUP = 0x1D0000109A,
  LVM_SETGROUPMETRICS = 0x1D0000109B,
  LVM_GETGROUPMETRICS = 0x1D0000109C,
  LVM_ENABLEGROUPVIEW = 0x1D0000109D,
  LVM_SORTGROUPS = 0x1D0000109E,
  LVM_INSERTGROUPSORTED = 0x1D0000109F,
  LVM_REMOVEALLGROUPS = 0x1D000010A0,
  LVM_HASGROUP = 0x1D000010A1,
  LVM_SETTILEVIEWINFO = 0x1D000010A2,
  LVM_GETTILEVIEWINFO = 0x1D000010A3,
  LVM_SETTILEINFO = 0x1D000010A4,
  LVM_GETTILEINFO = 0x1D000010A5,
  LVM_SETINSERTMARK = 0x1D000010A6,
  LVM_GETINSERTMARK = 0x1D000010A7,
  LVM_INSERTMARKHITTEST = 0x1D000010A8,
  LVM_GETINSERTMARKRECT = 0x1D000010A9,
  LVM_SETINSERTMARKCOLOR = 0x1D000010AA,
  LVM_GETINSERTMARKCOLOR = 0x1D000010AB,
  LVM_SETINFOTIP = 0x1D000010AD,
  LVM_GETSELECTEDCOLUMN = 0x1D000010AE,
  LVM_ISGROUPVIEWENABLED = 0x1D000010AF,
  LVM_GETOUTLINECOLOR = 0x1D000010B0,
  LVM_SETOUTLINECOLOR = 0x1D000010B1,
  LVM_CANCELEDITLABEL = 0x1D000010B3,
  LVM_MAPINDEXTOID = 0x1D000010B4,
  LVM_MAPIDTOINDEX = 0x1D000010B5,
  LVM_ISITEMVISIBLE = 0x1D000010B6,
  LVM_GETEMPTYTEXT = 0x1D000010CC,
  LVM_GETFOOTERRECT = 0x1D000010CD,
  LVM_GETFOOTERINFO = 0x1D000010CE,
  LVM_GETFOOTERITEMRECT = 0x1D000010CF,
  LVM_GETFOOTERITEM = 0x1D000010D0,
  LVM_GETITEMINDEXRECT = 0x1D000010D1,
  LVM_SETITEMINDEXSTATE = 0x1D000010D2,
  LVM_GETNEXTITEMINDEX = 0x1D000010D3,
  LVM_SETUNICODEFORMAT = 0x1D00002005,
  LVM_GETUNICODEFORMAT = 0x1D00002006,
  OCM__BASE = 0x1E00002000,
  OCM_CTLCOLOR = 0x1E00002019,
  OCM_DRAWITEM = 0x1E0000202B,
  OCM_MEASUREITEM = 0x1E0000202C,
  OCM_DELETEITEM = 0x1E0000202D,
  OCM_VKEYTOITEM = 0x1E0000202E,
  OCM_CHARTOITEM = 0x1E0000202F,
  OCM_COMPAREITEM = 0x1E00002039,
  OCM_NOTIFY = 0x1E0000204E,
  OCM_COMMAND = 0x1E00002111,
  OCM_HSCROLL = 0x1E00002114,
  OCM_VSCROLL = 0x1E00002115,
  OCM_CTLCOLORMSGBOX = 0x1E00002132,
  OCM_CTLCOLOREDIT = 0x1E00002133,
  OCM_CTLCOLORLISTBOX = 0x1E00002134,
  OCM_CTLCOLORBTN = 0x1E00002135,
  OCM_CTLCOLORDLG = 0x1E00002136,
  OCM_CTLCOLORSCROLLBAR = 0x1E00002137,
  OCM_CTLCOLORSTATIC = 0x1E00002138,
  OCM_PARENTNOTIFY = 0x1E00002210,
  WM_APP = 0x1E00008000,
  WM_RASDIALEVENT = 0x1E0000CCCD,
  CBEM_DELETEITEM = 0x1F00000144,
  CBEM_SETUNICODEFORMAT = 0x1F00002005,
  CBEM_GETUNICODEFORMAT = 0x1F00002006,
  IE_GETMODIFY = 0x20000000B8,
  IE_SETMODIFY = 0x20000000B9,
  IE_CANUNDO = 0x20000000C6,
  IE_UNDO = 0x20000000C7,
  IE_EMPTYUNDOBUFFER = 0x20000000CD,
  LVM_SETBKIMAGEW = 0x200000108A,
  MCIWNDM_GETDEVICEID = 0x2100000464,
  MCIWNDM_GETSTART = 0x2100000467,
  MCIWNDM_GETLENGTH = 0x2100000468,
  MCIWNDM_GETEND = 0x2100000469,
  MCIWNDM_EJECT = 0x210000046B,
  MCIWNDM_SETZOOM = 0x210000046C,
  MCIWNDM_SETVOLUME = 0x210000046E,
  MCIWNDM_GETVOLUME = 0x210000046F,
  MCIWNDM_SETSPEED = 0x2100000470,
  MCIWNDM_GETSPEED = 0x2100000471,
  MCIWNDM_SETREPEAT = 0x2100000472,
  MCIWNDM_GETREPEAT = 0x2100000473,
  MCIWNDM_PLAYFROM = 0x210000047A,
  MCIWNDM_SETTIMERS = 0x2100000481,
  MCIWNDM_SETACTIVETIMER = 0x2100000482,
  MCIWNDM_GETACTIVETIMER = 0x2100000484,
  MCIWNDM_CHANGESTYLES = 0x2100000487,
  MCIWNDM_GETSTYLES = 0x2100000488,
  MCIWNDM_GETALIAS = 0x2100000489,
  MCIWNDM_PLAYREVERSE = 0x210000048B,
  MCIWNDM_OPENINTERFACE = 0x2100000497,
  MCIWNDM_SETOWNER = 0x2100000498,
  MCIWNDM_SENDSTRINGA = 0x2200000465,
  MCIWNDM_GETPOSITIONA = 0x2200000466,
  MCIWNDM_GETMODEA = 0x220000046A,
  MCIWNDM_NEWA = 0x2200000486,
  MCIWNDM_RETURNSTRINGA = 0x220000048A,
  MCIWNDM_OPENA = 0x2200000499,
  MCIWNDM_SENDSTRINGW = 0x22000004C9,
  MCIWNDM_GETPOSITIONW = 0x22000004CA,
  MCIWNDM_GETMODEW = 0x22000004CE,
  MCIWNDM_NEWW = 0x22000004EA,
  MCIWNDM_RETURNSTRINGW = 0x22000004EE,
  MCIWNDM_OPENW = 0x22000004FC,
  MCIWNDM_NOTIFYPOS = 0x23000004C9,
  MCIWNDM_NOTIFYSIZE = 0x23000004CA,
  MSG_FTS_JUMP_HASH = 0x2400000420,
  MSG_FTS_GET_TITLE = 0x2400000422,
  PBM_SETBKCOLOR = 0x2400002001,
  RB_SETCOLORSCHEME = 0x2400002002,
  RB_GETCOLORSCHEME = 0x2400002003,
  RB_GETDROPTARGET = 0x2400002004,
  RB_SETUNICODEFORMAT = 0x2400002005,
  RB_GETUNICODEFORMAT = 0x2400002006,
  SB_SETUNICODEFORMAT = 0x2500002005,
  SB_GETUNICODEFORMAT = 0x2500002006,
  SB_SETBKCOLOR = 0x2600002001,
  STM_MSGMAX = 0x2700000174,
  TBM_SETUNICODEFORMAT = 0x2700002005,
  TBM_GETUNICODEFORMAT = 0x2700002006,
  TB_SETCOLORSCHEME = 0x2800002002,
  TB_GETCOLORSCHEME = 0x2800002003,
  TB_SETUNICODEFORMAT = 0x2800002005,
  TB_GETUNICODEFORMAT = 0x2800002006,
  UDM_SETUNICODEFORMAT = 0x2900002005,
  UDM_GETUNICODEFORMAT = 0x2900002006,
  WM_CAP_START = 0x2A00000400,
  WM_CAP_GET_CAPSTREAMPTR = 0x2A00000401,
  WM_CAP_SET_CALLBACK_ERRORA = 0x2A00000402,
  WM_CAP_SET_CALLBACK_STATUSA = 0x2A00000403,
  WM_CAP_SET_CALLBACK_YIELD = 0x2A00000404,
  WM_CAP_SET_CALLBACK_FRAME = 0x2A00000405,
  WM_CAP_SET_CALLBACK_VIDEOSTREAM = 0x2A00000406,
  WM_CAP_SET_CALLBACK_WAVESTREAM = 0x2A00000407,
  WM_CAP_GET_USER_DATA = 0x2A00000408,
  WM_CAP_SET_USER_DATA = 0x2A00000409,
  WM_CAP_DRIVER_CONNECT = 0x2A0000040A,
  WM_CAP_DRIVER_DISCONNECT = 0x2A0000040B,
  WM_CAP_DRIVER_GET_NAMEA = 0x2A0000040C,
  WM_CAP_DRIVER_GET_VERSIONA = 0x2A0000040D,
  WM_CAP_DRIVER_GET_CAPS = 0x2A0000040E,
  WM_CAP_FILE_SET_CAPTURE_FILEA = 0x2A00000414,
  WM_CAP_FILE_GET_CAPTURE_FILEA = 0x2A00000415,
  WM_CAP_FILE_SAVEASA = 0x2A00000417,
  WM_CAP_FILE_SAVEDIBA = 0x2A00000419,
  WM_CAP_FILE_ALLOCATE = 0x2B00000416,
  WM_CAP_FILE_SET_INFOCHUNK = 0x2B00000418,
  WM_CAP_EDIT_COPY = 0x2B0000041E,
  WM_CAP_SET_AUDIOFORMAT = 0x2B00000423,
  WM_CAP_GET_AUDIOFORMAT = 0x2B00000424,
  WM_CAP_DLG_VIDEOFORMAT = 0x2B00000429,
  WM_CAP_DLG_VIDEOSOURCE = 0x2B0000042A,
  WM_CAP_DLG_VIDEODISPLAY = 0x2B0000042B,
  WM_CAP_GET_VIDEOFORMAT = 0x2B0000042C,
  WM_CAP_SET_VIDEOFORMAT = 0x2B0000042D,
  WM_CAP_DLG_VIDEOCOMPRESSION = 0x2B0000042E,
  WM_CAP_SET_PREVIEW = 0x2B00000432,
  WM_CAP_SET_OVERLAY = 0x2B00000433,
  WM_CAP_SET_PREVIEWRATE = 0x2B00000434,
  WM_CAP_SET_SCALE = 0x2B00000435,
  WM_CAP_GET_STATUS = 0x2B00000436,
  WM_CAP_SET_SCROLL = 0x2B00000437,
  WM_CAP_GRAB_FRAME = 0x2B0000043C,
  WM_CAP_GRAB_FRAME_NOSTOP = 0x2B0000043D,
  WM_CAP_SEQUENCE = 0x2B0000043E,
  WM_CAP_SEQUENCE_NOFILE = 0x2B0000043F,
  WM_CAP_SET_SEQUENCE_SETUP = 0x2B00000440,
  WM_CAP_GET_SEQUENCE_SETUP = 0x2B00000441,
  WM_CAP_SET_MCI_DEVICEA = 0x2B00000442,
  WM_CAP_GET_MCI_DEVICEA = 0x2B00000443,
  WM_CAP_STOP = 0x2B00000444,
  WM_CAP_ABORT = 0x2B00000445,
  WM_CAP_SINGLE_FRAME_OPEN = 0x2B00000446,
  WM_CAP_SINGLE_FRAME_CLOSE = 0x2B00000447,
  WM_CAP_SINGLE_FRAME = 0x2B00000448,
  WM_CAP_PAL_OPENA = 0x2B00000450,
  WM_CAP_PAL_SAVEA = 0x2B00000451,
  WM_CAP_PAL_PASTE = 0x2B00000452,
  WM_CAP_PAL_AUTOCREATE = 0x2B00000453,
  WM_CAP_PAL_MANUALCREATE = 0x2B00000454,
  WM_CAP_SET_CALLBACK_CAPCONTROL = 0x2B00000455,
  WM_CAP_UNICODE_END = 0x2B000004B5,
  WM_CAP_END = 0x2B000004B5,
  WM_DDE_FIRST = 0x2C000003E0,
  WM_DDE_LAST = 0x2C000003E8,
  WM_DLGBORDER = 0x2C000011EF,
  WM_DLGSUBCLASS = 0x2C000011F0,
  WM_ADSPROP_NOTIFY_PAGEINIT = 0x2D0000084D,
  WM_ADSPROP_NOTIFY_PAGEHWND = 0x2D0000084E,
  WM_ADSPROP_NOTIFY_CHANGE = 0x2D0000084F,
  WM_ADSPROP_NOTIFY_APPLY = 0x2D00000850,
  WM_ADSPROP_NOTIFY_SETFOCUS = 0x2D00000851,
  WM_ADSPROP_NOTIFY_FOREGROUND = 0x2D00000852,
  WM_ADSPROP_NOTIFY_EXIT = 0x2D00000853,
  WM_ADSPROP_NOTIFY_ERROR = 0x2D00000856,
  WM_TOUCH = 0x2E00000240,
  WM_TOUCHHITTESTING = 0x2E0000024D,
  WM_DPICHANGED = 0x2E000002E0,
  WM_DPICHANGED_BEFOREPARENT = 0x2E000002E2,
  WM_DPICHANGED_AFTERPARENT = 0x2E000002E3,
  WM_CLIPBOARDUPDATE = 0x2E0000031D,
  WM_DWMCOMPOSITIONCHANGED = 0x2E0000031E,
  WM_DWMNCRENDERINGCHANGED = 0x2E0000031F,
  WM_DWMCOLORIZATIONCOLORCHANGED = 0x2E00000320,
  WM_DWMWINDOWMAXIMIZEDCHANGE = 0x2E00000321,
  WM_DWMSENDICONICTHUMBNAIL = 0x2E00000323,
  WM_DWMSENDICONICLIVEPREVIEWBITMAP = 0x2E00000326,
  WM_INPUT_DEVICE_CHANGE = 0x2F000000FE,
  WM_GESTURE = 0x2F00000119,
  WM_GESTURENOTIFY = 0x2F0000011A,
  WM_MOUSEHWHEEL = 0x2F0000020E,
  WM_POINTERDEVICECHANGE = 0x2F00000238,
  WM_POINTERDEVICEINRANGE = 0x2F00000239,
  WM_POINTERDEVICEOUTOFRANGE = 0x2F0000023A,
  WM_NCPOINTERUPDATE = 0x2F00000241,
  WM_NCPOINTERDOWN = 0x2F00000242,
  WM_NCPOINTERUP = 0x2F00000243,
  WM_POINTERUPDATE = 0x2F00000245,
  WM_POINTERDOWN = 0x2F00000246,
  WM_POINTERUP = 0x2F00000247,
  WM_POINTERENTER = 0x2F00000249,
  WM_POINTERLEAVE = 0x2F0000024A,
  WM_POINTERACTIVATE = 0x2F0000024B,
  WM_POINTERCAPTURECHANGED = 0x2F0000024C,
  WM_POINTERWHEEL = 0x2F0000024E,
  WM_POINTERHWHEEL = 0x2F0000024F,
  WM_POINTERROUTEDTO = 0x2F00000251,
  WM_POINTERROUTEDAWAY = 0x2F00000252,
  WM_POINTERROUTEDRELEASED = 0x2F00000253,
  WM_TABLET_ADDED = 0x2F000002C8,
  WM_TABLET_DELETED = 0x2F000002C9,
  WM_TABLET_FLICK = 0x2F000002CB,
  WM_TABLET_QUERYSYSTEMGESTURESTATUS = 0x2F000002CC,
  WM_GETDPISCALEDSIZE = 0x2F000002E4,
  WM_GETTITLEBARINFOEX = 0x2F0000033F,
};

struct std::array_int_7_
{
  int[7] _data;
};

struct QuestText
{
  std::string quest;
  std::string progress;
  std::string complete;
  std::string rollover;
  std::string log;
};

struct SeerHutText
{
  QuestText[10] quests;
  std::string seerHut;
  std::string questGuard;
};

struct std::vector_CObject_
{
  int8 allocator;
  CObject* first;
  CObject* last;
  CObject* end;
};

struct std::vector_CSprite_ptr_
{
  int8 allocator;
  CSprite** first;
  CSprite** last;
  CSprite** end;
};

struct std::vector_type_quest_ptr_
{
  int8 allocator;
  type_quest** first;
  type_quest** last;
  type_quest** end;
};

struct std::vector_TreasureData_
{
  int8 allocator;
  TreasureData* first;
  TreasureData* last;
  TreasureData* end;
};

struct std::vector_MonsterData_
{
  int8 allocator;
  MonsterData* first;
  MonsterData* last;
  MonsterData* end;
};

struct std::vector_BlackBoxData_
{
  int8 allocator;
  BlackBoxData* first;
  BlackBoxData* last;
  BlackBoxData* end;
};

struct std::vector_TSeerHut_
{
  int8 allocator;
  TSeerHut* first;
  TSeerHut* last;
  TSeerHut* end;
};

struct std::vector_TQuestGuard_
{
  int8 allocator;
  TQuestGuard* first;
  TQuestGuard* last;
  TQuestGuard* end;
};

struct std::vector_TTimedEvent_
{
  int8 allocator;
  TTimedEvent* first;
  TTimedEvent* last;
  TTimedEvent* end;
};

struct std::vector_TTownEvent_
{
  int8 allocator;
  TTownEvent* first;
  TTownEvent* last;
  TTownEvent* end;
};

struct std::vector_HeroPlaceholder_
{
  int8 allocator;
  HeroPlaceholder* first;
  HeroPlaceholder* last;
  HeroPlaceholder* end;
};

struct std::vector_TRandomDwelling_
{
  int8 allocator;
  TRandomDwelling* first;
  TRandomDwelling* last;
  TRandomDwelling* end;
};

struct slider::vftable_t : widget::vftable_t
{
  void (__thiscall *)(slider* SetResolution, int this);
  void (__thiscall *)(slider* SetResolution, int this);
  void (__thiscall *)(slider* SetResolution, int this);
  void (__thiscall *)(slider* SetResolution);
};

int (__fastcall *TSliderFunction)(int, heroWindow*);

struct LODFileDescriptor
{
  uint index;
  void* data;
};

struct LODResourceFiles
{
  LODFileDescriptor spriteLOD;
  LODFileDescriptor bitmapLOD;
  LODFileDescriptor soundLOD;
};

struct mapCellArtifact_RoE
{
  unsigned int32 price : 4;
  unsigned int32 guard : 9;
  unsigned int32 resource_price : 4;
  unsigned int32 guard_qty : 14;
  unsigned int32 custom : 1;
};

struct mapCellCorpse_RoE
{
  unsigned int32 id : 5;
  unsigned int32 artifact : 1;
  int32 has_treasure : 7;
  unsigned int32  : 1;
  unsigned int32  : 15;
};

struct mapCellMonster_RoE
{
  unsigned int32 qty : 12;
  unsigned int32 disposition : 5;
  unsigned int32 never_flee : 1;
  unsigned int32 dont_grow : 1;
  unsigned int32 index : 12;
  unsigned int32 custom : 1;
};

struct mapCellSeaChest_RoE
{
  unsigned int32 reward : 3;
  int32 artifact : 8;
  unsigned int32  : 19;
};

struct mapCellTreasureChest_RoE
{
  int32 artifact : 8;
  unsigned int32 is_artifact : 1;
  unsigned int32 gold_amount : 4;
  unsigned int32  : 19;
};

struct mapCellWagon_RoE
{
  unsigned int32 resource_amount : 5;
  unsigned int32 visited_bits : 8;
  unsigned int32 full : 1;
  unsigned int32 has_artifact : 1;
  int32 artifact : 8;
  unsigned int32 resource : 4;
  unsigned int32  : 5;
};

struct mapCellWarriorsTomb_RoE
{
  unsigned int32 full : 1;
  unsigned int32 visited_bits : 4;
  unsigned int32 artifact : 8;
  int32  : 8;
  unsigned int32  : 11;
};

struct CCombatChatEdit : CGameChatEdit { };

struct CHotspotWidget : widget { };

struct CAdvancedOption : widget
{
  Bitmap816* PlayerPanel;
  Bitmap816* PlayerFlag;
  CSprite* TownPix;
  TTownType TownType;
  int8* PlayerName;
  int8* Handicap;
  int8* ComputerOrPlayer;
  int Player;
  int StartingBonus;
  CSprite* StartingBonusPix;
  CSprite* HeroPortait;
  hero* Hero;
};

struct CAnimatedDlg::vftable_t : CTextDialog::vftable_t
{
  void (__thiscall *)(CTextDialog* CalcDimensions, int8* this, font* Setup, int* this, int* cText, int* pFont, int* winX);
  bool (__thiscall *)(CAnimatedDlg* CalcDimensions, int8* this, font* Setup, int* this, int* cText, int* pFont, int* winX);
};

struct type_progress_bar
{
  type_progress_bar::vftable_t* vftable;
  int maximum;
  int value;
};

struct type_map_creation_bar : type_progress_bar
{
  TWidgetVector widgets;
  heroWindow* parent;
  CSprite* progress_sprite;
  int num_progress_sprites;
};

struct type_progress_bar::vftable_t
{
  void* (__thiscall *)(type_progress_bar* scalar_deleting_destructor, uint this);
  int (__thiscall *)(type_progress_bar* scalar_deleting_destructor, int this);
  void (__thiscall *)(type_progress_bar* scalar_deleting_destructor, int this);
};

enum EGameDifficulty
{
  DIFFICULTY_EASY = 0x0,
  DIFFICULTY_NORMAL = 0x1,
  DIFFICULTY_HARD = 0x2,
  DIFFICULTY_EXPERT = 0x3,
  DIFFICULTY_IMPOSSIBLE = 0x4,
  DIFFICULTY_NONE = 0xFFFFFFFF,
  DIFFICULTY_MAX = 0x100000005,
};

enum EVictoryConditionType
{
  VICTORY_NONE = 0xFFFFFFFF,
  VICTORY_AQUIRE_ARTIFACT = 0x100000000,
  VICTORY_ACCUMULATE_CREATURES = 0x100000001,
  VICTORY_ACCUMULATE_RESOURCES = 0x100000002,
  VICTORY_UPGRADE_TOWN = 0x100000003,
  VICTORY_BUILD_HOLY_GRAIL_STRUCT = 0x100000004,
  VICTORY_DEFEAT_HERO = 0x100000005,
  VICTORY_CAPTURE_TOWN = 0x100000006,
  VICTORY_DEFEAT_MONSTER = 0x100000007,
  VICTORY_FLAG_ALL_CREATURE_GENERATORS = 0x100000008,
  VICTORY_FLAG_ALL_MINES = 0x100000009,
  VICTORY_TRANSPORT_ARTIFACT = 0x10000000A,
  VICTORY_DEFEAT_ALL_MONSTERS = 0x10000000B,
  VICTORY_SURVIVE_UNTIL_TIME_EXPIRES = 0x10000000C,
  MAX_VICTORY_CONDITIONS = 0x10000000D,
};

enum ELossConditionType
{
  LOSS_NONE = 0xFFFFFFFF,
  LOSS_LOSE_TOWN = 0x100000000,
  LOSS_LOSE_HERO = 0x100000001,
  LOSS_TIME_EXPIRES = 0x100000002,
  MAX_LOSS_CONDITIONS = 0x100000003,
};

enum EPlayerColor
{
  PLAYER_NONE = 0xFFFFFFFF,
  PLAYER_RED = 0x100000000,
  PLAYER_BLUE = 0x100000001,
  PLAYER_TAN = 0x100000002,
  PLAYER_GREEN = 0x100000003,
  PLAYER_ORANGE = 0x100000004,
  PLAYER_PURPLE = 0x100000005,
  PLAYER_TEAL = 0x100000006,
  PLAYER_PINK = 0x100000007,
  MAX_PLAYERS = 0x100000008,
};

enum EPlayerBit
{
  PLAYER_BIT_RED = 0x1,
  PLAYER_BIT_BLUE = 0x2,
  PLYAER_BIT_TAN = 0x4,
  PLAYER_BIT_GREEN = 0x8,
  PLAYER_BIT_ORANGE = 0x10,
  PLAYER_BIT_PURPLE = 0x20,
  PLAYER_BIT_TEAL = 0x40,
  PLAYER_BIT_PINK = 0xFFFFFF80,
};

struct TCreatureStack
{
  TCreatureType Creature;
  int16 numTroops;
  byte[2] gap6;
};

struct TSeerResourceReward
{
  EGameResource resType;
  uint resQty;
};

struct TSeerPrimarySkillReward
{
  TPrimarySkill skillType;
  uchar bonus;
  byte[3] gap5;
};

union TSeerReward::SeerRewardUnion
{
  int ExperienceBonus;
  int ManaBonus;
  uchar MoraleBonus;
  uchar LuckBonus;
  TSeerResourceReward ResourceReward;
  TSeerPrimarySkillReward PrimarySkillReward;
  SecondarySkillData SecondarySkillReward;
  TArtifact ArtifactReward;
  SpellID SpellReward;
  TCreatureStack CreatureReward;
};

struct SoundHeaderDescriptor
{
  SoundHeaderStruct** sounds;
  int* numSound;
  HANDLE* fileHandle;
};

struct SoundHeaders
{
  SoundHeaderDescriptor SoundHeader;
  SoundHeaderDescriptor SoundHeaderCD;
  SoundHeaderDescriptor SoundHeaderAB;
};

struct CTimer
{
  uint startTime;
  uint stopTime;
  uint elapsedTime;
  bool _IsRunning;
  bool enabled;
};

struct CHotSeatDlg : CHeroWindowEx
{
  textWidget*[8] edit;
  textWidget* m_rollover;
  THelpText[20] gHotSeatHelp;
};

struct t_net_file
{
  TAbstractFile::vftable_t* vftable;
  bool unknown;
  CNetMsg* buf;
  uint subtype;
  uint size;
};

enum ECombatAction
{
  COMBAT_ACTION_NONE = 0x0,
  COMBAT_ACTION_CAST_SPELL = 0x1,
  COMBAT_ACTION_ARMY_MOVE = 0x2,
  COMBAT_ACTION_ARMY_DEFENSE = 0x3,
  COMBAT_ACTION_RETREAT = 0x4,
  COMBAT_ACTION_SURRENDER = 0x5,
  COMBAT_ACTION_ARMY_MELEE_ATTACK = 0x6,
  COMBAT_ACTION_ARMY_SHOOT_ATTACK = 0x7,
  COMBAT_ACTION_ARMY_WAIT = 0x8,
  COMBAT_ACTION_CATAPULT_SHOOT = 0x9,
  COMBAT_ACTION_ARMY_CAST_SPELL = 0xA,
  COMBAT_ACTION_FIRST_AID_TENT_CURE = 0xB,
  COMBAT_ACTION_SKIP_TURN = 0xC,
  MAX_COMBAT_ACTIONS = 0xD,
};

enum message::EQualifiers
{
  QUALIFIER_KEY_SHIFT = 0x1,
  QUALIFIER_KEY_RSHIFT = 0x2,
  QUALIFIER_KEY_CTRL = 0x4,
  QUALIFIER_KEY_ALT = 0x20,
  QUALIFIER_RIGHT_CLICK = 0x200,
};

struct Bitmap816::vftable_t : resource::vftable_t
{
  void (__thiscall *)(Bitmap816* zBufferDraw, int this, int, int, int, ushort*, int, int, int);
};

union resource::vftable_union_t
{
  resource::vftable_t* resource_vftable;
  Bitmap816::vftable_t* Bitmap816_vftable;
};

struct townObjectProperties
{
  int16 frames;
  int16 x;
  int16 y;
};

enum MACRO_VK
{
  VK_LBUTTON = 0x1,
  VK_RBUTTON = 0x2,
  VK_CANCEL = 0x3,
  VK_MBUTTON = 0x4,
  VK_XBUTTON1 = 0x5,
  VK_XBUTTON2 = 0x6,
  VK_BACK = 0x8,
  VK_TAB = 0x9,
  VK_CLEAR = 0xC,
  VK_RETURN = 0xD,
  VK_SHIFT = 0x10,
  VK_CONTROL = 0x11,
  VK_MENU = 0x12,
  VK_PAUSE = 0x13,
  VK_CAPITAL = 0x14,
  VK_KANA = 0x15,
  VK_HANGEUL = 0x15,
  VK_HANGUL = 0x15,
  VK_JUNJA = 0x17,
  VK_FINAL = 0x18,
  VK_HANJA = 0x19,
  VK_KANJI = 0x19,
  VK_ESCAPE = 0x1B,
  VK_CONVERT = 0x1C,
  VK_NONCONVERT = 0x1D,
  VK_ACCEPT = 0x1E,
  VK_MODECHANGE = 0x1F,
  VK_SPACE = 0x20,
  VK_PRIOR = 0x21,
  VK_NEXT = 0x22,
  VK_END = 0x23,
  VK_HOME = 0x24,
  VK_LEFT = 0x25,
  VK_UP = 0x26,
  VK_RIGHT = 0x27,
  VK_DOWN = 0x28,
  VK_SELECT = 0x29,
  VK_PRINT = 0x2A,
  VK_EXECUTE = 0x2B,
  VK_SNAPSHOT = 0x2C,
  VK_INSERT = 0x2D,
  VK_DELETE = 0x2E,
  VK_HELP = 0x2F,
  VK_LWIN = 0x5B,
  VK_RWIN = 0x5C,
  VK_APPS = 0x5D,
  VK_SLEEP = 0x5F,
  VK_NUMPAD0 = 0x60,
  VK_NUMPAD1 = 0x61,
  VK_NUMPAD2 = 0x62,
  VK_NUMPAD3 = 0x63,
  VK_NUMPAD4 = 0x64,
  VK_NUMPAD5 = 0x65,
  VK_NUMPAD6 = 0x66,
  VK_NUMPAD7 = 0x67,
  VK_NUMPAD8 = 0x68,
  VK_NUMPAD9 = 0x69,
  VK_MULTIPLY = 0x6A,
  VK_ADD = 0x6B,
  VK_SEPARATOR = 0x6C,
  VK_SUBTRACT = 0x6D,
  VK_DECIMAL = 0x6E,
  VK_DIVIDE = 0x6F,
  VK_F1 = 0x70,
  VK_F2 = 0x71,
  VK_F3 = 0x72,
  VK_F4 = 0x73,
  VK_F5 = 0x74,
  VK_F6 = 0x75,
  VK_F7 = 0x76,
  VK_F8 = 0x77,
  VK_F9 = 0x78,
  VK_F10 = 0x79,
  VK_F11 = 0x7A,
  VK_F12 = 0x7B,
  VK_F13 = 0x7C,
  VK_F14 = 0x7D,
  VK_F15 = 0x7E,
  VK_F16 = 0x7F,
  VK_F17 = 0x80,
  VK_F18 = 0x81,
  VK_F19 = 0x82,
  VK_F20 = 0x83,
  VK_F21 = 0x84,
  VK_F22 = 0x85,
  VK_F23 = 0x86,
  VK_F24 = 0x87,
  VK_NUMLOCK = 0x90,
  VK_SCROLL = 0x91,
  VK_OEM_NEC_EQUAL = 0x92,
  VK_OEM_FJ_JISHO = 0x92,
  VK_OEM_FJ_MASSHOU = 0x93,
  VK_OEM_FJ_TOUROKU = 0x94,
  VK_OEM_FJ_LOYA = 0x95,
  VK_OEM_FJ_ROYA = 0x96,
  VK_LSHIFT = 0xA0,
  VK_RSHIFT = 0xA1,
  VK_LCONTROL = 0xA2,
  VK_RCONTROL = 0xA3,
  VK_LMENU = 0xA4,
  VK_RMENU = 0xA5,
  VK_BROWSER_BACK = 0xA6,
  VK_BROWSER_FORWARD = 0xA7,
  VK_BROWSER_REFRESH = 0xA8,
  VK_BROWSER_STOP = 0xA9,
  VK_BROWSER_SEARCH = 0xAA,
  VK_BROWSER_FAVORITES = 0xAB,
  VK_BROWSER_HOME = 0xAC,
  VK_VOLUME_MUTE = 0xAD,
  VK_VOLUME_DOWN = 0xAE,
  VK_VOLUME_UP = 0xAF,
  VK_MEDIA_NEXT_TRACK = 0xB0,
  VK_MEDIA_PREV_TRACK = 0xB1,
  VK_MEDIA_STOP = 0xB2,
  VK_MEDIA_PLAY_PAUSE = 0xB3,
  VK_LAUNCH_MAIL = 0xB4,
  VK_LAUNCH_MEDIA_SELECT = 0xB5,
  VK_LAUNCH_APP1 = 0xB6,
  VK_LAUNCH_APP2 = 0xB7,
  VK_OEM_1 = 0xBA,
  VK_OEM_PLUS = 0xBB,
  VK_OEM_COMMA = 0xBC,
  VK_OEM_MINUS = 0xBD,
  VK_OEM_PERIOD = 0xBE,
  VK_OEM_2 = 0xBF,
  VK_OEM_3 = 0xC0,
  VK_OEM_4 = 0xDB,
  VK_OEM_5 = 0xDC,
  VK_OEM_6 = 0xDD,
  VK_OEM_7 = 0xDE,
  VK_OEM_8 = 0xDF,
  VK_OEM_AX = 0xE1,
  VK_OEM_102 = 0xE2,
  VK_ICO_HELP = 0xE3,
  VK_ICO_00 = 0xE4,
  VK_PROCESSKEY = 0xE5,
  VK_ICO_CLEAR = 0xE6,
  VK_PACKET = 0xE7,
  VK_OEM_RESET = 0xE9,
  VK_OEM_JUMP = 0xEA,
  VK_OEM_PA1 = 0xEB,
  VK_OEM_PA2 = 0xEC,
  VK_OEM_PA3 = 0xED,
  VK_OEM_WSCTRL = 0xEE,
  VK_OEM_CUSEL = 0xEF,
  VK_OEM_ATTN = 0xF0,
  VK_OEM_FINISH = 0xF1,
  VK_OEM_COPY = 0xF2,
  VK_OEM_AUTO = 0xF3,
  VK_OEM_ENLW = 0xF4,
  VK_OEM_BACKTAB = 0xF5,
  VK_ATTN = 0xF6,
  VK_CRSEL = 0xF7,
  VK_EXSEL = 0xF8,
  VK_EREOF = 0xF9,
  VK_PLAY = 0xFA,
  VK_ZOOM = 0xFB,
  VK_NONAME = 0xFC,
  VK_PA1 = 0xFD,
  VK_OEM_CLEAR = 0xFE,
};

