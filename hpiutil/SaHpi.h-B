/*******************************************************************************
**
** FILE:
**   SaHpi.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum Platform Interface. It contains all of
**   the prototypes and type definitions. Note, this file was 
**   generated from the Platform Interface specification document.
**
** SPECIFICATION VERSION:
**   SAI-HPI-B.01.01
**
** DATE: 
**   Tue  Jun  1  2004  09:33
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**   The Specification and all worldwide copyrights therein are
**   the exclusive property of Licensor.  You may not remove, obscure, or
**   alter any copyright or other proprietary rights notices that are in or
**   on the copy of the Specification you download.  You must reproduce all
**   such notices on all copies of the Specification you make.  Licensor
**   may make changes to the Specification, or to items referenced therein,
**   at any time without notice.  Licensor is not obligated to support or
**   update the Specification. 
**   
**   Copyright(c) 2004, Service Availability(TM) Forum. All rights
**   reserved. 
**
*******************************************************************************/

#ifndef __SAHPI_H
#define __SAHPI_H

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                 Basic Data Types and Values                **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/* General Types - need to be specified correctly for the host architecture */

/* 
**   It is recommended that these types be defined such that the data sizes 
**   and alignment of each data type are as indicated.  The only requirement
**   for source compatibility is that the types be defined to be able to 
**   contain at least the required data (e.g., at least signed 8-bit values
**   must be contained in the data type defined as SaHpiInt8T, etc.)
**   Following the full recommendations for data size and alignment, however,
**   may promote more binary compatibility.
*/

/* The following definitions produce the recommended sizes and alignments
** using the gcc compiler for the i386 (IA-32) platform. 
**
**  Note, some recent versions of the gcc compiler exhibit an apparent bug
**  that makes the __attribute__ statements applied to the typdef's on
**  the 64-bit types below ineffective when those types are used in structures.
**  To workaround that bug, it may be required to add similar __attribute__
**  statements on the typedefs of the derived types SaHpiTimeT and
**  SaHpiTimeoutT, plus on individual 64-bit data items within structure and
**  union definitions in order to align the HPI structures as recommended.  
**  The structures and unions that contain 64-bit data items are:
**  SaHpiSensorReadingUnionT, SaHpiSensorDataFormatT, SaHpiEventT,
**  SaHpiAnnouncementT, SaHpiDomainInfoT, SaHpiAlarmT, SaHpiEventLogInfoT,
**  and SaHpiEventLogEntryT.  For more information, see:
**  https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=109911
*/


/* unsigned 8-bit data, 1-byte alignment   */
typedef unsigned char     SaHpiUint8T;   

/* unsigned 16-bit data, 2-byte alignment  */
typedef unsigned short    SaHpiUint16T;  

/* unsigned 32-bit data, 4-byte alignment  */
typedef unsigned int      SaHpiUint32T;  

/* unsigned 64-bit data, 8-byte alignment  */
typedef unsigned long long int  SaHpiUint64T __attribute__((__aligned__(8)));

/* signed 8-bit data, 1-byte alignment     */
typedef signed char       SaHpiInt8T;    

/* signed 16-bit data, 2-byte alignment    */
typedef signed short      SaHpiInt16T;   

/* signed 32-bit data, 4-byte alignment    */
typedef signed int        SaHpiInt32T;   

/* signed 64-bit data, 8-byte alignment    */
typedef signed long long int SaHpiInt64T __attribute__((__aligned__(8)));

/* 64-bit floating point, 8-byte alignment */
typedef double            SaHpiFloat64T __attribute__((__aligned__(8)));


typedef SaHpiUint8T     SaHpiBoolT;
#define SAHPI_TRUE      1  /* While SAHPI_TRUE = 1, any non-zero
                              value is also considered to be True
                              and HPI Users/Implementers of this 
                              specification should not test for
                              equality against SAHPI_TRUE. */

#define SAHPI_FALSE     0 

/* Platform, O/S, or Vendor dependent */
#define SAHPI_API
#define SAHPI_IN
#define SAHPI_OUT
#define SAHPI_INOUT

/* 
** Identifier for the manufacturer 
**
** This is the IANA-assigned private enterprise number for the
** manufacturer of the resource or FRU, or of the manufacturer
** defining an OEM control or event type. A list of current
** IANA-assigned private enterprise numbers may be obtained at
**
**     http://www.iana.org/assignments/enterprise-numbers
**
** If a manufacturer does not currently have an assigned number, one
** may be obtained by following the instructions located at
**
**     http://www.iana.org/cgi-bin/enterprise.pl
*/
typedef SaHpiUint32T SaHpiManufacturerIdT;
#define SAHPI_MANUFACTURER_ID_UNSPECIFIED (SaHpiManufacturerIdT)0

/* Version Types  */
typedef SaHpiUint32T SaHpiVersionT;

/*
** Interface Version
**
** The interface version is the version of the actual interface and not the 
** version of the implementation. It is a 24 bit value where 
** the most significant 8 bits represent the compatibility level
** (with letters represented as the corresponding numbers);
** the next 8 bits represent the major version number; and 
** the least significant 8 bits represent the minor version number.
*/
#define SAHPI_INTERFACE_VERSION (SaHpiVersionT)0x020101  /* B.01.01 */

/*
** Return Codes
**
** SaErrorT is defined in the HPI specification.  In the future a
** common SAF types definition may be created to contain this type. At
** that time, this typedef should be removed. Each of the return codes
** is defined in Section 4.1 of the specification.
*/
typedef SaHpiInt32T SaErrorT; /* Return code */

/*
** SA_OK: 
*/
#define SA_OK                          (SaErrorT)0x0000

/* This value is the base for all HPI-specific error codes. */
#define SA_HPI_ERR_BASE                -1000

#define SA_ERR_HPI_ERROR               (SaErrorT)(SA_HPI_ERR_BASE - 1)
#define SA_ERR_HPI_UNSUPPORTED_API     (SaErrorT)(SA_HPI_ERR_BASE - 2)
#define SA_ERR_HPI_BUSY                (SaErrorT)(SA_HPI_ERR_BASE - 3)
#define SA_ERR_HPI_INTERNAL_ERROR      (SaErrorT)(SA_HPI_ERR_BASE - 4)
#define SA_ERR_HPI_INVALID_CMD         (SaErrorT)(SA_HPI_ERR_BASE - 5)
#define SA_ERR_HPI_TIMEOUT             (SaErrorT)(SA_HPI_ERR_BASE - 6)
#define SA_ERR_HPI_OUT_OF_SPACE        (SaErrorT)(SA_HPI_ERR_BASE - 7)
#define SA_ERR_HPI_OUT_OF_MEMORY       (SaErrorT)(SA_HPI_ERR_BASE - 8)
#define SA_ERR_HPI_INVALID_PARAMS      (SaErrorT)(SA_HPI_ERR_BASE - 9)
#define SA_ERR_HPI_INVALID_DATA        (SaErrorT)(SA_HPI_ERR_BASE - 10)
#define SA_ERR_HPI_NOT_PRESENT         (SaErrorT)(SA_HPI_ERR_BASE - 11)
#define SA_ERR_HPI_NO_RESPONSE         (SaErrorT)(SA_HPI_ERR_BASE - 12)
#define SA_ERR_HPI_DUPLICATE           (SaErrorT)(SA_HPI_ERR_BASE - 13)
#define SA_ERR_HPI_INVALID_SESSION     (SaErrorT)(SA_HPI_ERR_BASE - 14)
#define SA_ERR_HPI_INVALID_DOMAIN      (SaErrorT)(SA_HPI_ERR_BASE - 15)
#define SA_ERR_HPI_INVALID_RESOURCE    (SaErrorT)(SA_HPI_ERR_BASE - 16)
#define SA_ERR_HPI_INVALID_REQUEST     (SaErrorT)(SA_HPI_ERR_BASE - 17)
#define SA_ERR_HPI_ENTITY_NOT_PRESENT  (SaErrorT)(SA_HPI_ERR_BASE - 18)
#define SA_ERR_HPI_READ_ONLY           (SaErrorT)(SA_HPI_ERR_BASE - 19)
#define SA_ERR_HPI_CAPABILITY          (SaErrorT)(SA_HPI_ERR_BASE - 20)
#define SA_ERR_HPI_UNKNOWN             (SaErrorT)(SA_HPI_ERR_BASE - 21)

/*
** Domain, Session and Resource Type Definitions
*/

/* Domain ID. */
typedef SaHpiUint32T SaHpiDomainIdT;

/* The SAHPI_UNSPECIFIED_DOMAIN_ID value is used to specify the default 
** domain.
*/
#define SAHPI_UNSPECIFIED_DOMAIN_ID (SaHpiDomainIdT) 0xFFFFFFFF

/* Session ID. */
typedef SaHpiUint32T SaHpiSessionIdT;

/* Resource identifier. */
typedef SaHpiUint32T SaHpiResourceIdT;

/* The SAHPI_UNSPECIFIED_RESOURCE_ID value is used to specify the Domain
** Event Log and to specify that there is no resource for such things as HPI 
** User events/alarms.  
*/
#define SAHPI_UNSPECIFIED_RESOURCE_ID (SaHpiResourceIdT) 0xFFFFFFFF

/* Table Related Type Definitions  */
typedef SaHpiUint32T SaHpiEntryIdT;
#define SAHPI_FIRST_ENTRY (SaHpiEntryIdT)0x00000000
#define SAHPI_LAST_ENTRY  (SaHpiEntryIdT)0xFFFFFFFF
#define SAHPI_ENTRY_UNSPECIFIED SAHPI_FIRST_ENTRY

/*
** Time Related Type Definitions
**
** An HPI time value represents the local time as the number of nanoseconds 
** from 00:00:00, January 1, 1970, in a 64-bit signed integer. This format
** is sufficient to represent times with nano-second resolution from the
** year 1678 to 2262. Every API which deals with time values must define
** the timezone used.
**
** It should be noted that although nano-second resolution is supported
** in the data type, the actual resolution provided by an implementation
** may be more limited than this.
**
** The value -2**63, which is 0x8000000000000000, is used to indicate
** "unknown/unspecified time".
** 
** Conversion to/from POSIX and other common time representations is
** relatively straightforward. The following code framgment converts 
** between SaHpiTimeT and time_t:
** 
**     time_t tt1, tt2;
**     SaHpiTimeT saHpiTime;
**     
**     time(&tt1);
**     saHpiTime = (SaHpiTimeT) tt1 * 1000000000;
**     tt2 = saHpiTime / 1000000000;
**
** The following fragment converts between SaHpiTimeT and a struct timeval:
**
**     struct timeval tv1, tv2;
**     SaHpiTimeT saHpiTime;
**     
**     gettimeofday(&tv1, NULL);
**     saHpiTime = (SaHpiTimeT) tv1.tv_sec * 1000000000 + tv1.tv_usec * 1000;
**     tv2.tv_sec = saHpiTime / 1000000000;
**     tv2.tv_usec = saHpiTime % 1000000000 / 1000;
**
** The following fragment converts between SaHpiTimeT and a struct timespec:
**
**     struct timespec ts1, ts2;
**     SaHpiTimeT saHpiTime;
**     
**     clock_gettime(CLOCK_REALTIME, &ts1);
**     saHpiTime = (SaHpiTimeT) ts1.tv_sec * 1000000000 + ts1.tv_nsec;
**     ts2.tv_sec = saHpiTime / 1000000000;
**     ts2.tv_nsec = saHpiTime % 1000000000;
**
** Note, however, that since time_t is (effectively) universally 32 bits,
** all of these conversions will cease to work on January 18, 2038.
** 
** Some subsystems may need the flexibility to report either absolute or
** relative (eg. to system boot) times. This will typically be in the
** case of a board which may or may not, depending on the system setup,
** have an idea of absolute time. For example, some boards may have
** "time of day" clocks which start at zero, and never get set to the
** time of day.
**
** In these cases, times which represent "current" time (in events, for
** example) can be reported based on the clock value, whether it has been
** set to the actual date/time, or whether it represents the elapsed time
** since boot. If it is the time since boot, the value will be (for 27
** years) less than 0x0C00000000000000, which is Mon May 26 16:58:48 1997.
** If the value is greater than this, then it can be assumed to be an
** absolute time.
**
** There is no practical need within the interface for expressing dates prior 
** to the publication of this specification (which is more than five years
** after the "break point" between relative and absolute time). Thus, in all
** instances a time value should be interpreted as "relative" times if the 
** value is less than or equal to SAHPI_TIME_MAX_RELATIVE (but not equal to 
** SAHPI_TIME_UNSPECIFIED which always means the time is not available), or 
** "absolute" times if the value is greater than SAHPI_TIME_MAX_RELATIVE. 
*/
typedef SaHpiInt64T SaHpiTimeT;    /* Time in nanoseconds */

/* Unspecified or unknown time */
#define SAHPI_TIME_UNSPECIFIED     (SaHpiTimeT) 0x8000000000000000LL

/* Maximum time that can be specified as relative */
#define SAHPI_TIME_MAX_RELATIVE    (SaHpiTimeT) 0x0C00000000000000LL
typedef SaHpiInt64T SaHpiTimeoutT; /* Timeout in nanoseconds */

/* Non-blocking call */
#define SAHPI_TIMEOUT_IMMEDIATE    (SaHpiTimeoutT) 0x0000000000000000LL

/* Blocking call, wait indefinitely for call to complete */
#define SAHPI_TIMEOUT_BLOCK        (SaHpiTimeoutT) -1LL

/*
** Language
**
** This enumeration lists all of the languages that can be associated with text.
**
** SAHPI_LANG_UNDEF indicates that the language is unspecified or
** unknown.
*/
typedef enum {
    SAHPI_LANG_UNDEF = 0, SAHPI_LANG_AFAR, SAHPI_LANG_ABKHAZIAN,
    SAHPI_LANG_AFRIKAANS, SAHPI_LANG_AMHARIC, SAHPI_LANG_ARABIC,
    SAHPI_LANG_ASSAMESE, SAHPI_LANG_AYMARA, SAHPI_LANG_AZERBAIJANI,
    SAHPI_LANG_BASHKIR, SAHPI_LANG_BYELORUSSIAN, SAHPI_LANG_BULGARIAN,
    SAHPI_LANG_BIHARI, SAHPI_LANG_BISLAMA, SAHPI_LANG_BENGALI,
    SAHPI_LANG_TIBETAN, SAHPI_LANG_BRETON, SAHPI_LANG_CATALAN,
    SAHPI_LANG_CORSICAN, SAHPI_LANG_CZECH, SAHPI_LANG_WELSH,
    SAHPI_LANG_DANISH, SAHPI_LANG_GERMAN, SAHPI_LANG_BHUTANI,
    SAHPI_LANG_GREEK, SAHPI_LANG_ENGLISH, SAHPI_LANG_ESPERANTO,
    SAHPI_LANG_SPANISH, SAHPI_LANG_ESTONIAN, SAHPI_LANG_BASQUE,
    SAHPI_LANG_PERSIAN, SAHPI_LANG_FINNISH, SAHPI_LANG_FIJI,
    SAHPI_LANG_FAEROESE, SAHPI_LANG_FRENCH, SAHPI_LANG_FRISIAN,
    SAHPI_LANG_IRISH, SAHPI_LANG_SCOTSGAELIC, SAHPI_LANG_GALICIAN,
    SAHPI_LANG_GUARANI, SAHPI_LANG_GUJARATI, SAHPI_LANG_HAUSA,
    SAHPI_LANG_HINDI, SAHPI_LANG_CROATIAN, SAHPI_LANG_HUNGARIAN,
    SAHPI_LANG_ARMENIAN, SAHPI_LANG_INTERLINGUA, SAHPI_LANG_INTERLINGUE,
    SAHPI_LANG_INUPIAK, SAHPI_LANG_INDONESIAN, SAHPI_LANG_ICELANDIC,
    SAHPI_LANG_ITALIAN, SAHPI_LANG_HEBREW, SAHPI_LANG_JAPANESE,
    SAHPI_LANG_YIDDISH, SAHPI_LANG_JAVANESE, SAHPI_LANG_GEORGIAN,
    SAHPI_LANG_KAZAKH, SAHPI_LANG_GREENLANDIC, SAHPI_LANG_CAMBODIAN,
    SAHPI_LANG_KANNADA, SAHPI_LANG_KOREAN, SAHPI_LANG_KASHMIRI,
    SAHPI_LANG_KURDISH, SAHPI_LANG_KIRGHIZ, SAHPI_LANG_LATIN,
    SAHPI_LANG_LINGALA, SAHPI_LANG_LAOTHIAN, SAHPI_LANG_LITHUANIAN,
    SAHPI_LANG_LATVIANLETTISH, SAHPI_LANG_MALAGASY, SAHPI_LANG_MAORI,
    SAHPI_LANG_MACEDONIAN, SAHPI_LANG_MALAYALAM, SAHPI_LANG_MONGOLIAN,
    SAHPI_LANG_MOLDAVIAN, SAHPI_LANG_MARATHI, SAHPI_LANG_MALAY,
    SAHPI_LANG_MALTESE, SAHPI_LANG_BURMESE, SAHPI_LANG_NAURU,
    SAHPI_LANG_NEPALI, SAHPI_LANG_DUTCH, SAHPI_LANG_NORWEGIAN,
    SAHPI_LANG_OCCITAN, SAHPI_LANG_AFANOROMO, SAHPI_LANG_ORIYA,
    SAHPI_LANG_PUNJABI, SAHPI_LANG_POLISH, SAHPI_LANG_PASHTOPUSHTO,
    SAHPI_LANG_PORTUGUESE, SAHPI_LANG_QUECHUA, SAHPI_LANG_RHAETOROMANCE,
    SAHPI_LANG_KIRUNDI, SAHPI_LANG_ROMANIAN, SAHPI_LANG_RUSSIAN,
    SAHPI_LANG_KINYARWANDA, SAHPI_LANG_SANSKRIT, SAHPI_LANG_SINDHI,
    SAHPI_LANG_SANGRO, SAHPI_LANG_SERBOCROATIAN, SAHPI_LANG_SINGHALESE,
    SAHPI_LANG_SLOVAK, SAHPI_LANG_SLOVENIAN, SAHPI_LANG_SAMOAN,
    SAHPI_LANG_SHONA, SAHPI_LANG_SOMALI, SAHPI_LANG_ALBANIAN,
    SAHPI_LANG_SERBIAN, SAHPI_LANG_SISWATI, SAHPI_LANG_SESOTHO,
    SAHPI_LANG_SUDANESE, SAHPI_LANG_SWEDISH, SAHPI_LANG_SWAHILI,
    SAHPI_LANG_TAMIL, SAHPI_LANG_TELUGU, SAHPI_LANG_TAJIK,
    SAHPI_LANG_THAI, SAHPI_LANG_TIGRINYA, SAHPI_LANG_TURKMEN,
    SAHPI_LANG_TAGALOG, SAHPI_LANG_SETSWANA, SAHPI_LANG_TONGA,
    SAHPI_LANG_TURKISH, SAHPI_LANG_TSONGA, SAHPI_LANG_TATAR,
    SAHPI_LANG_TWI, SAHPI_LANG_UKRAINIAN, SAHPI_LANG_URDU,
    SAHPI_LANG_UZBEK, SAHPI_LANG_VIETNAMESE, SAHPI_LANG_VOLAPUK,
    SAHPI_LANG_WOLOF, SAHPI_LANG_XHOSA, SAHPI_LANG_YORUBA,
    SAHPI_LANG_CHINESE, SAHPI_LANG_ZULU
} SaHpiLanguageT;

/*
** Text Buffers
**
** These structures are used for defining the type of data in the text buffer 
** and the length of the buffer. Text buffers are used in the inventory data,
** RDR, RPT, etc. for variable length strings of data.
**
** The encoding of the Data field in the SaHpiTextBufferT structure is defined
** by the value of the DataType field in the buffer.  The following table
** describes the various encodings:
**
**    DataType                     Encoding
**    --------                     --------
**
**   SAHPI_TL_TYPE_UNICODE         16-bit Unicode, least significant byte first.
**                                 Buffer must contain even number of bytes.
**
**   SAHPI_TL_TYPE_BCDPLUS         8-bit ASCII, '0'-'9' or space, dash, period,
**                                 colon, comma, or underscore only.
**
**   SAHPI_TL_TYPE_ASCII6          8-bit ASCII, reduced set, 0x20=0x5f only.
**
**   SAHPI_TL_TYPE_TEXT            8-bit ASCII+Latin 1
**
**   SAHPI_TL_TYPE_BINARY          8-bit bytes, any values legal
**
** Note: "ASCII+Latin 1" is derived from the first 256 characters of 
**        Unicode 2.0. The first 256 codes of Unicode follow ISO 646 (ASCII)
**        and ISO 8859/1 (Latin 1). The Unicode "C0 Controls and Basic Latin"
**        set defines the first 128 8-bit characters (00h-7Fh) and the 
**        "C1 Controls and Latin 1 Supplement" defines the second 128 (80h-FFh).
**
** Note: The SAHPI_TL_TYPE_BCDPLUS and SAHPI_TL_TYPE_ASCII6 encodings
**        use normal ASCII character encodings, but restrict the allowed
**        characters to a subset of the entire ASCII character set. These
**        encodings are used when the target device contains restrictions
**        on which characters it can store or display.  SAHPI_TL_TYPE_BCDPLUS 
**        data may be stored externally as 4-bit values, and 
**        SAHPI_TL_TYPE_ASCII6 may be stored externally as 6-bit values. 
**        But, regardless of how the data is stored externally, it is 
**        encoded as 8-bit ASCII in the SaHpiTextBufferT structure passed 
**        across the HPI.
*/

#define SAHPI_MAX_TEXT_BUFFER_LENGTH  255

typedef enum {
    SAHPI_TL_TYPE_UNICODE = 0,     /* 2-byte UNICODE characters; DataLength 
                                     must be even. */
    SAHPI_TL_TYPE_BCDPLUS,        /* String of ASCII characters, '0'-'9', space,
                                     dash, period, colon, comma or underscore
                                     ONLY */
    SAHPI_TL_TYPE_ASCII6,         /* Reduced ASCII character set: 0x20-0x5F 
                                     ONLY */
    SAHPI_TL_TYPE_TEXT,           /* ASCII+Latin 1 */
    SAHPI_TL_TYPE_BINARY          /* Binary data, any values legal */
} SaHpiTextTypeT;

typedef struct {
    SaHpiTextTypeT DataType;
    SaHpiLanguageT Language;      /* Language the text is in. */
    SaHpiUint8T    DataLength;    /* Bytes used in Data buffer  */ 
    SaHpiUint8T    Data[SAHPI_MAX_TEXT_BUFFER_LENGTH];  /* Data buffer */
} SaHpiTextBufferT;

/* 
** Instrument Id
**
** The following data type is used for all management instrument identifiers -
** sensor numbers, control numbers, watchdog timer numbers, etc.
**
*/

typedef SaHpiUint32T SaHpiInstrumentIdT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                           Entities                         **********
**********                                                            **********
********************************************************************************
*******************************************************************************/
/*
** Entity Types
**
** Entities are used to associate specific hardware components with sensors, 
** controls, watchdogs, or resources. Entities are defined with an entity 
** type enumeration, and an entity location number (to identify 
** the physical location of a particular type of entity).
**
** Entities are uniquely identified in a system with an ordered series of
** Entity Type / Entity Location pairs called an "Entity Path". Each subsequent
** Entity Type/Entity Location in the path is the next higher "containing"
** entity. The "root" of the Entity Path (the outermost level of containment)
** is designated with an Entity Type of SAHPI_ENT_ROOT if the entire Entity Path
** is fewer than SAHPI_MAX_ENTITY_PATH entries in length.
**
** Enumerated Entity Types include those types enumerated by the IPMI Consortium
** for IPMI-managed entities, as well as additional types defined by the 
** HPI specification. Room is left in the enumeration for the inclusion of
** Entity Types taken from other lists, if needed in the future.
*/
/* Base values for entity types from various sources. */
#define SAHPI_ENT_IPMI_GROUP 0
#define SAHPI_ENT_SAFHPI_GROUP 0x10000
#define SAHPI_ENT_ROOT_VALUE 0xFFFF
typedef enum
{
    SAHPI_ENT_UNSPECIFIED = SAHPI_ENT_IPMI_GROUP, 
    SAHPI_ENT_OTHER,
    SAHPI_ENT_UNKNOWN,
    SAHPI_ENT_PROCESSOR,
    SAHPI_ENT_DISK_BAY,            /* Disk or disk bay  */
    SAHPI_ENT_PERIPHERAL_BAY,
    SAHPI_ENT_SYS_MGMNT_MODULE,    /* System management module  */
    SAHPI_ENT_SYSTEM_BOARD,        /* Main system board, may also be
                                     processor board and/or internal
                                     expansion board */
    SAHPI_ENT_MEMORY_MODULE,       /* Board holding memory devices */
    SAHPI_ENT_PROCESSOR_MODULE,    /* Holds processors, use this
                                     designation when processors are not
                                     mounted on system board */
    SAHPI_ENT_POWER_SUPPLY,        /* Main power supply (supplies) for the
                                     system */
    SAHPI_ENT_ADD_IN_CARD,
    SAHPI_ENT_FRONT_PANEL_BOARD,   /* Control panel  */
    SAHPI_ENT_BACK_PANEL_BOARD,
    SAHPI_ENT_POWER_SYSTEM_BOARD,
    SAHPI_ENT_DRIVE_BACKPLANE,
    SAHPI_ENT_SYS_EXPANSION_BOARD, /* System internal expansion board
                                     (contains expansion slots). */
    SAHPI_ENT_OTHER_SYSTEM_BOARD,  /* Part of board set          */
    SAHPI_ENT_PROCESSOR_BOARD,     /* Holds 1 or more processors. Includes 
                                     boards that hold SECC modules) */
    SAHPI_ENT_POWER_UNIT,          /* Power unit / power domain (typically
                                     used as a pre-defined logical entity
                                     for grouping power supplies)*/
    SAHPI_ENT_POWER_MODULE,        /* Power module / DC-to-DC converter.
                                     Use this value for internal
                                     converters. Note: You should use
                                     entity ID (power supply) for the
                                     main power supply even if the main
                                     supply is a DC-to-DC converter */
    SAHPI_ENT_POWER_MGMNT,         /* Power management/distribution 
                                     board */
    SAHPI_ENT_CHASSIS_BACK_PANEL_BOARD,
    SAHPI_ENT_SYSTEM_CHASSIS,
    SAHPI_ENT_SUB_CHASSIS,
    SAHPI_ENT_OTHER_CHASSIS_BOARD,
    SAHPI_ENT_DISK_DRIVE_BAY,
    SAHPI_ENT_PERIPHERAL_BAY_2,
    SAHPI_ENT_DEVICE_BAY,
    SAHPI_ENT_COOLING_DEVICE,      /* Fan/cooling device */
    SAHPI_ENT_COOLING_UNIT,        /* Can be used as a pre-defined logical
                                     entity for grouping fans or other
                                     cooling devices. */
    SAHPI_ENT_INTERCONNECT,        /* Cable / interconnect */
    SAHPI_ENT_MEMORY_DEVICE,       /* This Entity ID should be used for
                                     replaceable memory devices, e.g.
                                     DIMM/SIMM. It is recommended that
                                     Entity IDs not be used for 
                                     individual non-replaceable memory
                                     devices. Rather, monitoring and
                                     error reporting should be associated
                                     with the FRU [e.g. memory card]
                                     holding the memory. */
    SAHPI_ENT_SYS_MGMNT_SOFTWARE, /* System Management Software  */
    SAHPI_ENT_BIOS,
    SAHPI_ENT_OPERATING_SYSTEM,
    SAHPI_ENT_SYSTEM_BUS,
    SAHPI_ENT_GROUP,              /* This is a logical entity for use with
                                    Entity Association records. It is
                                    provided to allow a sensor data
                                    record to point to an entity-
                                    association record when there is no
                                    appropriate pre-defined logical
                                    entity for the entity grouping.
                                    This Entity should not be used as a
                                    physical entity. */
    SAHPI_ENT_REMOTE,             /* Out of band management communication
                                    device */
    SAHPI_ENT_EXTERNAL_ENVIRONMENT,
    SAHPI_ENT_BATTERY,
    SAHPI_ENT_CHASSIS_SPECIFIC    = SAHPI_ENT_IPMI_GROUP + 0x90,
    SAHPI_ENT_BOARD_SET_SPECIFIC  = SAHPI_ENT_IPMI_GROUP + 0xB0,
    SAHPI_ENT_OEM_SYSINT_SPECIFIC = SAHPI_ENT_IPMI_GROUP + 0xD0,
    SAHPI_ENT_ROOT = SAHPI_ENT_ROOT_VALUE,
    SAHPI_ENT_RACK = SAHPI_ENT_SAFHPI_GROUP,
    SAHPI_ENT_SUBRACK,
    SAHPI_ENT_COMPACTPCI_CHASSIS,
    SAHPI_ENT_ADVANCEDTCA_CHASSIS,
    SAHPI_ENT_RACK_MOUNTED_SERVER,
    SAHPI_ENT_SYSTEM_BLADE,
    SAHPI_ENT_SWITCH,                    /* Network switch, such as a
                                            rack-mounted ethernet or fabric
                                            switch. */
    SAHPI_ENT_SWITCH_BLADE,              /* Network switch, as above, but in
                                            a bladed system. */
    SAHPI_ENT_SBC_BLADE,
    SAHPI_ENT_IO_BLADE,
    SAHPI_ENT_DISK_BLADE,
    SAHPI_ENT_DISK_DRIVE,
    SAHPI_ENT_FAN,
    SAHPI_ENT_POWER_DISTRIBUTION_UNIT,
    SAHPI_ENT_SPEC_PROC_BLADE,           /* Special Processing Blade,
                                            including DSP */
    SAHPI_ENT_IO_SUBBOARD,               /* I/O Sub-Board, including
                                            PMC I/O board */
    SAHPI_ENT_SBC_SUBBOARD,              /* SBC Sub-Board, including PMC
                                            SBC board */
    SAHPI_ENT_ALARM_MANAGER,             /* Chassis alarm manager board */
    SAHPI_ENT_SHELF_MANAGER,             /* Blade-based shelf manager */
    SAHPI_ENT_DISPLAY_PANEL,             /* Display panel, such as an 
                                            alarm display panel. */
    SAHPI_ENT_SUBBOARD_CARRIER_BLADE,    /* Includes PMC Carrier Blade --
                                            Use only if "carrier" is only
                                            function of blade. Else use
                                            primary function (SBC_BLADE,
                                            DSP_BLADE, etc.). */
    SAHPI_ENT_PHYSICAL_SLOT              /* Indicates the physical slot into
                                            which a blade is inserted. */
} SaHpiEntityTypeT;

typedef SaHpiUint32T SaHpiEntityLocationT;

typedef struct {
    SaHpiEntityTypeT     EntityType;
    SaHpiEntityLocationT EntityLocation;
} SaHpiEntityT;


#define SAHPI_MAX_ENTITY_PATH 16

typedef struct {
    SaHpiEntityT  Entry[SAHPI_MAX_ENTITY_PATH];
} SaHpiEntityPathT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                        Events, part 1                      **********
**********                                                            **********
********************************************************************************
*******************************************************************************/
/*
** Category
**
** Sensor events contain an event category and event state. Depending on the 
** event category, the event states take on different meanings for events 
** generated by specific sensors.
**
** It is recommended that implementations map their sensor specific
** event categories into the set of categories listed here.  When such a mapping
** is impractical or impossible, the SAHPI_EC_SENSOR_SPECIFIC category should 
** be used. 
**
** The SAHPI_EC_GENERIC category can be used for discrete sensors which have 
** state meanings other than those identified with other event categories.
*/
typedef SaHpiUint8T SaHpiEventCategoryT;

#define SAHPI_EC_UNSPECIFIED     (SaHpiEventCategoryT)0x00 /* Unspecified */
#define SAHPI_EC_THRESHOLD       (SaHpiEventCategoryT)0x01 /* Threshold 
                                                              events */
#define SAHPI_EC_USAGE           (SaHpiEventCategoryT)0x02 /* Usage state
                                                              events */
#define SAHPI_EC_STATE           (SaHpiEventCategoryT)0x03 /* Generic state
                                                              events */
#define SAHPI_EC_PRED_FAIL       (SaHpiEventCategoryT)0x04 /* Predictive fail 
                                                              events */
#define SAHPI_EC_LIMIT           (SaHpiEventCategoryT)0x05 /* Limit events */
#define SAHPI_EC_PERFORMANCE     (SaHpiEventCategoryT)0x06 /* Performance 
                                                              events    */
#define SAHPI_EC_SEVERITY        (SaHpiEventCategoryT)0x07 /* Severity  
                                                              indicating
                                                              events */
#define SAHPI_EC_PRESENCE        (SaHpiEventCategoryT)0x08 /* Device presence
                                                              events */
#define SAHPI_EC_ENABLE          (SaHpiEventCategoryT)0x09 /* Device enabled
                                                              events */
#define SAHPI_EC_AVAILABILITY    (SaHpiEventCategoryT)0x0A /* Availability
                                                              state events */
#define SAHPI_EC_REDUNDANCY      (SaHpiEventCategoryT)0x0B /* Redundancy 
                                                              state events */
#define SAHPI_EC_SENSOR_SPECIFIC (SaHpiEventCategoryT)0x7E /* Sensor-
                                                              specific events */
#define SAHPI_EC_GENERIC         (SaHpiEventCategoryT)0x7F /* OEM defined
                                                              events */

/*
** Event States
**
** The following event states are specified relative to the categories listed 
** above. The event types are only valid for their given category. Each set of 
** events is labeled as to which category it belongs to.
** Each event will have only one event state associated with it. When retrieving
** the event status or event enabled status a bit mask of all applicable event 
** states is used. Similarly, when setting the event enabled status a bit mask 
** of all applicable event states is used.
*/
typedef SaHpiUint16T SaHpiEventStateT;

/* 
** SaHpiEventCategoryT == <any> 
*/
#define SAHPI_ES_UNSPECIFIED (SaHpiEventStateT)0x0000

/* 
** SaHpiEventCategoryT == SAHPI_EC_THRESHOLD 
** When using these event states, the event state should match
** the event severity (for example SAHPI_ES_LOWER_MINOR should have an 
** event severity of SAHPI_MINOR).
*/
#define SAHPI_ES_LOWER_MINOR (SaHpiEventStateT)0x0001
#define SAHPI_ES_LOWER_MAJOR (SaHpiEventStateT)0x0002
#define SAHPI_ES_LOWER_CRIT  (SaHpiEventStateT)0x0004
#define SAHPI_ES_UPPER_MINOR (SaHpiEventStateT)0x0008
#define SAHPI_ES_UPPER_MAJOR (SaHpiEventStateT)0x0010
#define SAHPI_ES_UPPER_CRIT  (SaHpiEventStateT)0x0020

/* SaHpiEventCategoryT == SAHPI_EC_USAGE */
#define SAHPI_ES_IDLE   (SaHpiEventStateT)0x0001
#define SAHPI_ES_ACTIVE (SaHpiEventStateT)0x0002
#define SAHPI_ES_BUSY   (SaHpiEventStateT)0x0004

/* SaHpiEventCategoryT == SAHPI_EC_STATE */
#define SAHPI_ES_STATE_DEASSERTED (SaHpiEventStateT)0x0001
#define SAHPI_ES_STATE_ASSERTED   (SaHpiEventStateT)0x0002

/* SaHpiEventCategoryT == SAHPI_EC_PRED_FAIL */
#define SAHPI_ES_PRED_FAILURE_DEASSERT (SaHpiEventStateT)0x0001
#define SAHPI_ES_PRED_FAILURE_ASSERT   (SaHpiEventStateT)0x0002

/* SaHpiEventCategoryT == SAHPI_EC_LIMIT */
#define SAHPI_ES_LIMIT_NOT_EXCEEDED (SaHpiEventStateT)0x0001
#define SAHPI_ES_LIMIT_EXCEEDED     (SaHpiEventStateT)0x0002

/* SaHpiEventCategoryT == SAHPI_EC_PERFORMANCE */
#define SAHPI_ES_PERFORMANCE_MET   (SaHpiEventStateT)0x0001
#define SAHPI_ES_PERFORMANCE_LAGS  (SaHpiEventStateT)0x0002

/*
** SaHpiEventCategoryT == SAHPI_EC_SEVERITY 
** When using these event states, the event state should match
** the event severity 
*/
#define SAHPI_ES_OK                  (SaHpiEventStateT)0x0001
#define SAHPI_ES_MINOR_FROM_OK       (SaHpiEventStateT)0x0002
#define SAHPI_ES_MAJOR_FROM_LESS     (SaHpiEventStateT)0x0004
#define SAHPI_ES_CRITICAL_FROM_LESS  (SaHpiEventStateT)0x0008
#define SAHPI_ES_MINOR_FROM_MORE     (SaHpiEventStateT)0x0010
#define SAHPI_ES_MAJOR_FROM_CRITICAL (SaHpiEventStateT)0x0020
#define SAHPI_ES_CRITICAL            (SaHpiEventStateT)0x0040
#define SAHPI_ES_MONITOR             (SaHpiEventStateT)0x0080
#define SAHPI_ES_INFORMATIONAL       (SaHpiEventStateT)0x0100

/* SaHpiEventCategoryT == SAHPI_EC_PRESENCE */
#define SAHPI_ES_ABSENT  (SaHpiEventStateT)0x0001
#define SAHPI_ES_PRESENT (SaHpiEventStateT)0x0002

/* SaHpiEventCategoryT == SAHPI_EC_ENABLE */
#define SAHPI_ES_DISABLED (SaHpiEventStateT)0x0001
#define SAHPI_ES_ENABLED  (SaHpiEventStateT)0x0002

/* SaHpiEventCategoryT == SAHPI_EC_AVAILABILITY */
#define SAHPI_ES_RUNNING       (SaHpiEventStateT)0x0001
#define SAHPI_ES_TEST          (SaHpiEventStateT)0x0002
#define SAHPI_ES_POWER_OFF     (SaHpiEventStateT)0x0004
#define SAHPI_ES_ON_LINE       (SaHpiEventStateT)0x0008
#define SAHPI_ES_OFF_LINE      (SaHpiEventStateT)0x0010
#define SAHPI_ES_OFF_DUTY      (SaHpiEventStateT)0x0020
#define SAHPI_ES_DEGRADED      (SaHpiEventStateT)0x0040
#define SAHPI_ES_POWER_SAVE    (SaHpiEventStateT)0x0080
#define SAHPI_ES_INSTALL_ERROR (SaHpiEventStateT)0x0100

/* SaHpiEventCategoryT == SAHPI_EC_REDUNDANCY */
#define SAHPI_ES_FULLY_REDUNDANT                  (SaHpiEventStateT)0x0001
#define SAHPI_ES_REDUNDANCY_LOST                  (SaHpiEventStateT)0x0002
#define SAHPI_ES_REDUNDANCY_DEGRADED              (SaHpiEventStateT)0x0004
#define SAHPI_ES_REDUNDANCY_LOST_SUFFICIENT_RESOURCES \
                                                  (SaHpiEventStateT)0x0008
#define SAHPI_ES_NON_REDUNDANT_SUFFICIENT_RESOURCES \
                                                  (SaHpiEventStateT)0x0010
#define SAHPI_ES_NON_REDUNDANT_INSUFFICIENT_RESOURCES \
                                                  (SaHpiEventStateT)0x0020
#define SAHPI_ES_REDUNDANCY_DEGRADED_FROM_FULL    (SaHpiEventStateT)0x0040
#define SAHPI_ES_REDUNDANCY_DEGRADED_FROM_NON     (SaHpiEventStateT)0x0080

/*
** SaHpiEventCategoryT == SAHPI_EC_GENERIC || SAHPI_EC_SENSOR_SPECIFIC
** These event states are implementation-specific.
*/
#define SAHPI_ES_STATE_00  (SaHpiEventStateT)0x0001
#define SAHPI_ES_STATE_01  (SaHpiEventStateT)0x0002
#define SAHPI_ES_STATE_02  (SaHpiEventStateT)0x0004
#define SAHPI_ES_STATE_03  (SaHpiEventStateT)0x0008
#define SAHPI_ES_STATE_04  (SaHpiEventStateT)0x0010
#define SAHPI_ES_STATE_05  (SaHpiEventStateT)0x0020
#define SAHPI_ES_STATE_06  (SaHpiEventStateT)0x0040
#define SAHPI_ES_STATE_07  (SaHpiEventStateT)0x0080
#define SAHPI_ES_STATE_08  (SaHpiEventStateT)0x0100
#define SAHPI_ES_STATE_09  (SaHpiEventStateT)0x0200
#define SAHPI_ES_STATE_10  (SaHpiEventStateT)0x0400
#define SAHPI_ES_STATE_11  (SaHpiEventStateT)0x0800
#define SAHPI_ES_STATE_12  (SaHpiEventStateT)0x1000
#define SAHPI_ES_STATE_13  (SaHpiEventStateT)0x2000
#define SAHPI_ES_STATE_14  (SaHpiEventStateT)0x4000

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                           Sensors                          **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/* Sensor Number */
typedef SaHpiInstrumentIdT SaHpiSensorNumT;
/* The following specifies the named range for standard sensor numbers. */
#define SAHPI_STANDARD_SENSOR_MIN   (SaHpiSensorNumT)0x00000100
#define SAHPI_STANDARD_SENSOR_MAX   (SaHpiSensorNumT)0x000001FF

/* Type of Sensor */
typedef enum {
    SAHPI_TEMPERATURE = 0x01,
    SAHPI_VOLTAGE,
    SAHPI_CURRENT,
    SAHPI_FAN,
    SAHPI_PHYSICAL_SECURITY,
    SAHPI_PLATFORM_VIOLATION,
    SAHPI_PROCESSOR,
    SAHPI_POWER_SUPPLY,
    SAHPI_POWER_UNIT,
    SAHPI_COOLING_DEVICE,
    SAHPI_OTHER_UNITS_BASED_SENSOR,
    SAHPI_MEMORY,
    SAHPI_DRIVE_SLOT,
    SAHPI_POST_MEMORY_RESIZE,
    SAHPI_SYSTEM_FW_PROGRESS,
    SAHPI_EVENT_LOGGING_DISABLED,
    SAHPI_RESERVED1,
    SAHPI_SYSTEM_EVENT,
    SAHPI_CRITICAL_INTERRUPT,
    SAHPI_BUTTON,
    SAHPI_MODULE_BOARD,
    SAHPI_MICROCONTROLLER_COPROCESSOR,
    SAHPI_ADDIN_CARD,
    SAHPI_CHASSIS,
    SAHPI_CHIP_SET,
    SAHPI_OTHER_FRU,
    SAHPI_CABLE_INTERCONNECT,
    SAHPI_TERMINATOR,
    SAHPI_SYSTEM_BOOT_INITIATED,
    SAHPI_BOOT_ERROR,
    SAHPI_OS_BOOT,
    SAHPI_OS_CRITICAL_STOP,
    SAHPI_SLOT_CONNECTOR,
    SAHPI_SYSTEM_ACPI_POWER_STATE,
    SAHPI_RESERVED2,
    SAHPI_PLATFORM_ALERT,
    SAHPI_ENTITY_PRESENCE,
    SAHPI_MONITOR_ASIC_IC,
    SAHPI_LAN,
    SAHPI_MANAGEMENT_SUBSYSTEM_HEALTH,
    SAHPI_BATTERY,
    SAHPI_OPERATIONAL = 0xA0,
    SAHPI_OEM_SENSOR=0xC0
}  SaHpiSensorTypeT;

/*
** Sensor Reading Type
**
** These definitions list the available data types that can be
** used for sensor readings. 
**
*/

#define SAHPI_SENSOR_BUFFER_LENGTH 32

typedef enum {
      SAHPI_SENSOR_READING_TYPE_INT64,
      SAHPI_SENSOR_READING_TYPE_UINT64,
      SAHPI_SENSOR_READING_TYPE_FLOAT64,
      SAHPI_SENSOR_READING_TYPE_BUFFER    /* 32 byte array. The format of
                                             the buffer is implementation-
                                             specific. Sensors that use
                                             this reading type may not have
                                             thresholds that are settable
                                             or readable. */
} SaHpiSensorReadingTypeT;

typedef union {
    SaHpiInt64T          SensorInt64;
    SaHpiUint64T         SensorUint64;
    SaHpiFloat64T        SensorFloat64; 
    SaHpiUint8T          SensorBuffer[SAHPI_SENSOR_BUFFER_LENGTH];
} SaHpiSensorReadingUnionT;

/*
** Sensor Reading
**
** The sensor reading data structure is returned from a call to get 
** sensor reading. The structure is also used when setting and getting sensor 
** threshold values and reporting sensor ranges.
** 
** IsSupported is set when a sensor reading/threshold value is available.  
** Otherwise, if no reading or threshold is supported, this flag is set to
** False.
** 
*/

typedef struct {
      SaHpiBoolT                  IsSupported;
      SaHpiSensorReadingTypeT     Type;
      SaHpiSensorReadingUnionT    Value;
} SaHpiSensorReadingT;


/* Sensor Event Mask Actions - used with saHpiSensorEventMasksSet() */

typedef enum {
    SAHPI_SENS_ADD_EVENTS_TO_MASKS,
    SAHPI_SENS_REMOVE_EVENTS_FROM_MASKS
} SaHpiSensorEventMaskActionT;

/* Value to use for AssertEvents or DeassertEvents parameter
   in saHpiSensorEventMasksSet() to set or clear all supported
   event states for a sensor in the mask */

#define SAHPI_ALL_EVENT_STATES (SaHpiEventStateT)0xFFFF

/*
** Threshold Values
** This structure encompasses all of the thresholds that can be set.
** These are set and read with the same units as sensors report in
** saHpiSensorReadingGet().  When hysteresis is not constant over the 
** range of sensor values, it is calculated at the nominal sensor reading, 
** as given in the Range field of the sensor RDR.  
**
** Thresholds are required to be set in-order (such that the setting for
** UpCritical is greater than or equal to the setting for UpMajor, etc.).*/

typedef struct {
    SaHpiSensorReadingT LowCritical;      /* Lower Critical Threshold */
    SaHpiSensorReadingT LowMajor;         /* Lower Major Threshold */
    SaHpiSensorReadingT LowMinor;         /* Lower Minor Threshold */
    SaHpiSensorReadingT UpCritical;       /* Upper critical Threshold */
    SaHpiSensorReadingT UpMajor;          /* Upper major Threshold */
    SaHpiSensorReadingT UpMinor;          /* Upper minor Threshold */
    SaHpiSensorReadingT PosThdHysteresis; /* Positive Threshold Hysteresis */
    SaHpiSensorReadingT NegThdHysteresis; /* Negative Threshold Hysteresis */
}SaHpiSensorThresholdsT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                  Sensor Resource Data Records              **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
**  Sensor Range
** Sensor range values can include minimum, maximum, normal minimum, normal 
** maximum, and nominal values. 
**
** Sensor thresholds cannot be set outside of the range defined by SAHPI_SRF_MIN
** through SAHPI_SRF_MAX, if these limits are present (as indicated by the 
** SaHpiSensorRangeFlagsT).  If the MIN limit is not present, no lower bound 
** will be enforced on sensor thresholds.  If the MAX limit is not present, no  
** upper bound will be enforced on sensor thresholds. 
*/
typedef SaHpiUint8T SaHpiSensorRangeFlagsT;
#define SAHPI_SRF_MIN        (SaHpiSensorRangeFlagsT)0x10 
#define SAHPI_SRF_MAX        (SaHpiSensorRangeFlagsT)0x08 
#define SAHPI_SRF_NORMAL_MIN (SaHpiSensorRangeFlagsT)0x04
#define SAHPI_SRF_NORMAL_MAX (SaHpiSensorRangeFlagsT)0x02 
#define SAHPI_SRF_NOMINAL    (SaHpiSensorRangeFlagsT)0x01

typedef struct {
    SaHpiSensorRangeFlagsT  Flags;
    SaHpiSensorReadingT     Max;
    SaHpiSensorReadingT     Min;
    SaHpiSensorReadingT     Nominal;
    SaHpiSensorReadingT     NormalMax;
    SaHpiSensorReadingT     NormalMin;
} SaHpiSensorRangeT;

/*
** Sensor Units
** This is a list of all the sensor units supported by HPI.
*/
typedef enum {
    SAHPI_SU_UNSPECIFIED = 0, SAHPI_SU_DEGREES_C, SAHPI_SU_DEGREES_F,
    SAHPI_SU_DEGREES_K, SAHPI_SU_VOLTS, SAHPI_SU_AMPS,
    SAHPI_SU_WATTS, SAHPI_SU_JOULES, SAHPI_SU_COULOMBS,
    SAHPI_SU_VA, SAHPI_SU_NITS, SAHPI_SU_LUMEN,
    SAHPI_SU_LUX, SAHPI_SU_CANDELA, SAHPI_SU_KPA,
    SAHPI_SU_PSI, SAHPI_SU_NEWTON, SAHPI_SU_CFM,
    SAHPI_SU_RPM, SAHPI_SU_HZ, SAHPI_SU_MICROSECOND,
    SAHPI_SU_MILLISECOND, SAHPI_SU_SECOND, SAHPI_SU_MINUTE,
    SAHPI_SU_HOUR, SAHPI_SU_DAY, SAHPI_SU_WEEK,
    SAHPI_SU_MIL, SAHPI_SU_INCHES, SAHPI_SU_FEET,
    SAHPI_SU_CU_IN, SAHPI_SU_CU_FEET, SAHPI_SU_MM,
    SAHPI_SU_CM, SAHPI_SU_M, SAHPI_SU_CU_CM,
    SAHPI_SU_CU_M, SAHPI_SU_LITERS, SAHPI_SU_FLUID_OUNCE,
    SAHPI_SU_RADIANS, SAHPI_SU_STERADIANS, SAHPI_SU_REVOLUTIONS,
    SAHPI_SU_CYCLES, SAHPI_SU_GRAVITIES, SAHPI_SU_OUNCE,
    SAHPI_SU_POUND, SAHPI_SU_FT_LB, SAHPI_SU_OZ_IN,
    SAHPI_SU_GAUSS, SAHPI_SU_GILBERTS, SAHPI_SU_HENRY,
    SAHPI_SU_MILLIHENRY, SAHPI_SU_FARAD, SAHPI_SU_MICROFARAD,
    SAHPI_SU_OHMS, SAHPI_SU_SIEMENS, SAHPI_SU_MOLE,
    SAHPI_SU_BECQUEREL, SAHPI_SU_PPM, SAHPI_SU_RESERVED,
    SAHPI_SU_DECIBELS, SAHPI_SU_DBA, SAHPI_SU_DBC,
    SAHPI_SU_GRAY, SAHPI_SU_SIEVERT, SAHPI_SU_COLOR_TEMP_DEG_K,
    SAHPI_SU_BIT, SAHPI_SU_KILOBIT, SAHPI_SU_MEGABIT,
    SAHPI_SU_GIGABIT, SAHPI_SU_BYTE, SAHPI_SU_KILOBYTE,
    SAHPI_SU_MEGABYTE, SAHPI_SU_GIGABYTE, SAHPI_SU_WORD,
    SAHPI_SU_DWORD, SAHPI_SU_QWORD, SAHPI_SU_LINE,
    SAHPI_SU_HIT, SAHPI_SU_MISS, SAHPI_SU_RETRY,
    SAHPI_SU_RESET, SAHPI_SU_OVERRUN, SAHPI_SU_UNDERRUN,
    SAHPI_SU_COLLISION, SAHPI_SU_PACKETS, SAHPI_SU_MESSAGES,
    SAHPI_SU_CHARACTERS, SAHPI_SU_ERRORS, SAHPI_SU_CORRECTABLE_ERRORS,
    SAHPI_SU_UNCORRECTABLE_ERRORS
} SaHpiSensorUnitsT;

/*
** Modifier Unit Use
** This type defines how the modifier unit is used. For example: base unit == 
** meter, modifier unit == seconds, and modifier unit use == 
** SAHPI_SMUU_BASIC_OVER_MODIFIER. The resulting unit would be meters per
** second.
*/
typedef enum {
    SAHPI_SMUU_NONE = 0,
    SAHPI_SMUU_BASIC_OVER_MODIFIER,  /* Basic Unit / Modifier Unit */
    SAHPI_SMUU_BASIC_TIMES_MODIFIER  /* Basic Unit * Modifier Unit */
} SaHpiSensorModUnitUseT;

/*
** Data Format
** When IsSupported is False, the sensor does not support data readings
** (it only supports event states).  A False setting for this flag 
** indicates that the rest of the structure is not meaningful.
**  
** This structure encapsulates all of the various types that make up the 
** definition of sensor data. For reading type of
** SAHPI_SENSOR_READING_TYPE_BUFFER, the rest of the structure
** (beyond ReadingType) is not meaningful.
**  
** The Accuracy Factor is expressed as a floating point percentage
** (e.g. 0.05 = 5%) and represents statistically how close the measured 
** reading is to the actual value. It is an interpreted value that 
** figures in all sensor accuracies, resolutions, and tolerances. 
*/

typedef struct {
    SaHpiBoolT                 IsSupported;    /* Indicates if sensor data 
                                                   readings are supported.*/
    SaHpiSensorReadingTypeT    ReadingType;    /* Type of value for sensor 
                                                   reading. */
    SaHpiSensorUnitsT          BaseUnits;      /* Base units (meters, etc.)  */
    SaHpiSensorUnitsT          ModifierUnits;  /* Modifier unit (second, etc.)*/
    SaHpiSensorModUnitUseT     ModifierUse;    /* Modifier use(m/sec, etc.)  */
    SaHpiBoolT                 Percentage;     /* Is value a percentage */
    SaHpiSensorRangeT          Range;          /* Valid range of sensor */
    SaHpiFloat64T              AccuracyFactor; /* Accuracy */
} SaHpiSensorDataFormatT;

/*
** Threshold Support
**
** These types define what threshold values are readable and writable. 
** Thresholds are read/written in the same ReadingType as is used for sensor
** readings.  
*/
typedef SaHpiUint8T SaHpiSensorThdMaskT;
#define SAHPI_STM_LOW_MINOR      (SaHpiSensorThdMaskT)0x01
#define SAHPI_STM_LOW_MAJOR      (SaHpiSensorThdMaskT)0x02
#define SAHPI_STM_LOW_CRIT       (SaHpiSensorThdMaskT)0x04
#define SAHPI_STM_UP_MINOR       (SaHpiSensorThdMaskT)0x08
#define SAHPI_STM_UP_MAJOR       (SaHpiSensorThdMaskT)0x10
#define SAHPI_STM_UP_CRIT        (SaHpiSensorThdMaskT)0x20
#define SAHPI_STM_UP_HYSTERESIS  (SaHpiSensorThdMaskT)0x40
#define SAHPI_STM_LOW_HYSTERESIS (SaHpiSensorThdMaskT)0x80

typedef struct {
    SaHpiBoolT            IsAccessible; /* True if the sensor 
                                           supports readable or writable
                                           thresholds. If False,
                                           rest of structure is not
                                           meaningful. Sensors that have the 
                                           IsAccessible flag set must also 
                                           support the threshold event category
                                           A sensor of reading type SAHPI_
                                           SENSOR_READING_TYPE_BUFFER cannot
                                           have accessible thresholds.*/ 
    SaHpiSensorThdMaskT   ReadThold;    /* Readable thresholds */
    SaHpiSensorThdMaskT   WriteThold;   /* Writable thresholds */
    SaHpiBoolT            Nonlinear;    /* If this flag is set, hysteresis
                                           values are calculated at the nominal
                                           sensor value. */
} SaHpiSensorThdDefnT;

/*
** Event Control
**
** This type defines how sensor event messages can be controlled (can be turned
** off and on for each type of event, etc.).
*/
typedef enum {
    SAHPI_SEC_PER_EVENT = 0,    /* Event message control per event,
                                   or by entire sensor; sensor event enable
                                   status can be changed, and assert/deassert
                                   masks can be changed */
    SAHPI_SEC_READ_ONLY_MASKS,  /* Control for entire sensor only; sensor
                                   event enable status can be changed, but
                                   assert/deassert masks cannot be changed */
    SAHPI_SEC_READ_ONLY         /* Event control not supported; sensor event
                                   enable status cannot be changed and
                                   assert/deassert masks cannot be changed */
} SaHpiSensorEventCtrlT;

/*
** Record
**
** This is the sensor resource data record which describes all of the static 
** data associated with a sensor.
*/
typedef struct {
    SaHpiSensorNumT         Num;           /* Sensor Number/Index */
    SaHpiSensorTypeT        Type;          /* General Sensor Type */
    SaHpiEventCategoryT     Category;      /* Event category */
    SaHpiBoolT              EnableCtrl;    /* True if HPI User can enable
                                              or disable sensor via
                                              saHpiSensorEnableSet() */
    SaHpiSensorEventCtrlT   EventCtrl;     /* How events can be controlled */
    SaHpiEventStateT        Events;        /* Bit mask of event states 
                                              supported */
    SaHpiSensorDataFormatT  DataFormat;    /* Format of the data */
    SaHpiSensorThdDefnT     ThresholdDefn; /* Threshold Definition */
    SaHpiUint32T            Oem;           /* Reserved for OEM use */
} SaHpiSensorRecT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                      Aggregate Status                      **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/* These are the default sensor numbers for aggregate status. */
#define SAHPI_DEFAGSENS_OPER (SaHpiSensorNumT)0x00000100
#define SAHPI_DEFAGSENS_PWR  (SaHpiSensorNumT)0x00000101
#define SAHPI_DEFAGSENS_TEMP (SaHpiSensorNumT)0x00000102

/* The following specifies the named range for aggregate status. */
#define SAHPI_DEFAGSENS_MIN  (SaHpiSensorNumT)0x00000100
#define SAHPI_DEFAGSENS_MAX  (SaHpiSensorNumT)0x0000010F

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                           Controls                         **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/* Control Number  */
typedef SaHpiInstrumentIdT SaHpiCtrlNumT;

/*
** Type of Control
**
** This enumerated type defines the different types of generic controls.
*/
typedef enum {
    SAHPI_CTRL_TYPE_DIGITAL = 0x00,
    SAHPI_CTRL_TYPE_DISCRETE,
    SAHPI_CTRL_TYPE_ANALOG,
    SAHPI_CTRL_TYPE_STREAM,
    SAHPI_CTRL_TYPE_TEXT,
    SAHPI_CTRL_TYPE_OEM = 0xC0
} SaHpiCtrlTypeT;

/*
** Control State Type Definitions
*/

/*
** Digital Control State Definition
**
** Defines the types of digital control states.
** Any of the four states may be set using saHpiControlSet().
** Only ON or OFF are appropriate returns from saHpiControlGet().
** (PULSE_ON and PULSE_OFF are transitory and end in OFF and ON states, 
** respectively.)
** OFF - the control is off
** ON  - the control is on
** PULSE_OFF - the control is briefly turned off, and then turned back on
** PULSE_ON  - the control is briefly turned on, and then turned back off
**
*/
typedef enum {
    SAHPI_CTRL_STATE_OFF = 0,
    SAHPI_CTRL_STATE_ON,
    SAHPI_CTRL_STATE_PULSE_OFF,
    SAHPI_CTRL_STATE_PULSE_ON
} SaHpiCtrlStateDigitalT;

/*
** Discrete Control State Definition
*/
typedef SaHpiUint32T SaHpiCtrlStateDiscreteT;

/*
** Analog Control State Definition
*/
typedef SaHpiInt32T  SaHpiCtrlStateAnalogT;

/*
** Stream Control State Definition
*/
#define SAHPI_CTRL_MAX_STREAM_LENGTH 4
typedef struct { 
    SaHpiBoolT   Repeat;       /* Repeat flag */
    SaHpiUint32T StreamLength; /* Length of the data, in bytes, 
                                 stored in the stream. */
    SaHpiUint8T  Stream[SAHPI_CTRL_MAX_STREAM_LENGTH];
} SaHpiCtrlStateStreamT;

/*
** Text Control State Definition
*/
typedef SaHpiUint8T SaHpiTxtLineNumT;

/* Reserved number for sending output to all lines */
#define SAHPI_TLN_ALL_LINES (SaHpiTxtLineNumT)0x00 

typedef struct {
    SaHpiTxtLineNumT    Line; /* Operate on line # */ 
    SaHpiTextBufferT    Text; /* Text to display */
} SaHpiCtrlStateTextT;

/*
** OEM Control State Definition
*/
#define SAHPI_CTRL_MAX_OEM_BODY_LENGTH 255
typedef struct {
    SaHpiManufacturerIdT MId;
    SaHpiUint8T BodyLength;  
    SaHpiUint8T Body[SAHPI_CTRL_MAX_OEM_BODY_LENGTH]; /* OEM Specific */
} SaHpiCtrlStateOemT;

typedef union {
    SaHpiCtrlStateDigitalT  Digital;
    SaHpiCtrlStateDiscreteT Discrete;
    SaHpiCtrlStateAnalogT   Analog;
    SaHpiCtrlStateStreamT   Stream;
    SaHpiCtrlStateTextT     Text;
    SaHpiCtrlStateOemT      Oem;
} SaHpiCtrlStateUnionT;

typedef struct {
    SaHpiCtrlTypeT          Type;       /* Type of control */
    SaHpiCtrlStateUnionT    StateUnion; /* Data for control type */
} SaHpiCtrlStateT;
/*
** Control Mode Type Definition
**
** Controls may be in either AUTO mode or MANUAL mode.
** 
*/
typedef enum {
    SAHPI_CTRL_MODE_AUTO,
    SAHPI_CTRL_MODE_MANUAL
} SaHpiCtrlModeT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                 Control Resource Data Records              **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** Output Type
**
**  This enumeration defines the what the control's output will be.
*/
typedef enum {
    SAHPI_CTRL_GENERIC = 0,
    SAHPI_CTRL_LED,
    SAHPI_CTRL_FAN_SPEED,
    SAHPI_CTRL_DRY_CONTACT_CLOSURE,
    SAHPI_CTRL_POWER_SUPPLY_INHIBIT,
    SAHPI_CTRL_AUDIBLE,
    SAHPI_CTRL_FRONT_PANEL_LOCKOUT,
    SAHPI_CTRL_POWER_INTERLOCK,
    SAHPI_CTRL_POWER_STATE,
    SAHPI_CTRL_LCD_DISPLAY,
    SAHPI_CTRL_OEM
} SaHpiCtrlOutputTypeT;

/*
** Specific Record Types
** These types represent the specific types of control resource data records.
*/
typedef struct {
    SaHpiCtrlStateDigitalT Default;
} SaHpiCtrlRecDigitalT;

typedef struct {
    SaHpiCtrlStateDiscreteT Default;
} SaHpiCtrlRecDiscreteT;

typedef struct {
    SaHpiCtrlStateAnalogT  Min;    /* Minimum Value */
    SaHpiCtrlStateAnalogT  Max;    /* Maximum Value */
    SaHpiCtrlStateAnalogT  Default;
} SaHpiCtrlRecAnalogT;

typedef struct {
   SaHpiCtrlStateStreamT  Default;
} SaHpiCtrlRecStreamT;

typedef struct {
    SaHpiUint8T             MaxChars; /* Maximum chars per line. 
                                         If the control DataType is
                                         SAHPI_TL_TYPE_UNICODE, there will
                                         be two bytes required for each
                                         character.  This field reports the
                                         number of characters per line- not the
                                         number of bytes.  MaxChars must not be
                                         larger than the number of characters
                                         that can be placed in a single
                                         SaHpiTextBufferT structure. */
    SaHpiUint8T             MaxLines; /* Maximum # of lines */
    SaHpiLanguageT          Language; /* Language Code */
    SaHpiTextTypeT          DataType; /* Permitted Data */
    SaHpiCtrlStateTextT     Default;
} SaHpiCtrlRecTextT;

#define SAHPI_CTRL_OEM_CONFIG_LENGTH 10
typedef struct {
    SaHpiManufacturerIdT    MId;
    SaHpiUint8T             ConfigData[SAHPI_CTRL_OEM_CONFIG_LENGTH];
    SaHpiCtrlStateOemT      Default;
} SaHpiCtrlRecOemT;

typedef union {
    SaHpiCtrlRecDigitalT  Digital;
    SaHpiCtrlRecDiscreteT Discrete;
    SaHpiCtrlRecAnalogT   Analog;
    SaHpiCtrlRecStreamT   Stream;
    SaHpiCtrlRecTextT     Text;
    SaHpiCtrlRecOemT      Oem;
} SaHpiCtrlRecUnionT;

/* 
** Default Control Mode Structure
** This structure tells an HPI User if the control comes up in Auto mode or
** in Manual mode, by default.  It also indicates if the mode can be 
** changed (using saHpiControlSet()).  When ReadOnly is False, the mode
** can be changed from its default setting; otherwise attempting to 
** change the mode will result in an error.
*/
typedef struct {
    SaHpiCtrlModeT          Mode; /* Auto or Manual */
    SaHpiBoolT              ReadOnly; /* Indicates if mode is read-only */
} SaHpiCtrlDefaultModeT;

/*
** Record Definition
** Definition of the control resource data record.
*/
typedef struct {
    SaHpiCtrlNumT         Num;         /* Control Number/Index */
    SaHpiCtrlOutputTypeT  OutputType;
    SaHpiCtrlTypeT        Type;        /* Type of control */
    SaHpiCtrlRecUnionT    TypeUnion;   /* Specific control record */
    SaHpiCtrlDefaultModeT DefaultMode; /*Indicates if the control comes up
                                         in Auto or Manual mode. */
    SaHpiBoolT            WriteOnly;   /* Indicates if the control is 
                                          write-only. */
    SaHpiUint32T          Oem;         /* Reserved for OEM use */
} SaHpiCtrlRecT;


/*******************************************************************************
********************************************************************************
**********                                                            **********
**********               Inventory Data Repositories                  **********
**********                                                            **********
********************************************************************************
*******************************************************************************/
/*
** These structures are used to read and write inventory data to entity
** inventory data repositories within a resource. 
*/
/*
** Inventory Data Repository ID
** Identifier for an inventory data repository.
*/
typedef SaHpiInstrumentIdT SaHpiIdrIdT;
#define SAHPI_DEFAULT_INVENTORY_ID (SaHpiIdrIdT)0x00000000

/* Inventory Data Area type definitions */
typedef enum {
    SAHPI_IDR_AREATYPE_INTERNAL_USE = 0xB0,
    SAHPI_IDR_AREATYPE_CHASSIS_INFO,
    SAHPI_IDR_AREATYPE_BOARD_INFO,
    SAHPI_IDR_AREATYPE_PRODUCT_INFO,
    SAHPI_IDR_AREATYPE_OEM = 0xC0,
    SAHPI_IDR_AREATYPE_UNSPECIFIED = 0xFF
} SaHpiIdrAreaTypeT;

/* Inventory Data Field type definitions */
typedef enum {
    SAHPI_IDR_FIELDTYPE_CHASSIS_TYPE,
    SAHPI_IDR_FIELDTYPE_MFG_DATETIME,
    SAHPI_IDR_FIELDTYPE_MANUFACTURER,
    SAHPI_IDR_FIELDTYPE_PRODUCT_NAME,
    SAHPI_IDR_FIELDTYPE_PRODUCT_VERSION,
    SAHPI_IDR_FIELDTYPE_SERIAL_NUMBER,
    SAHPI_IDR_FIELDTYPE_PART_NUMBER,
    SAHPI_IDR_FIELDTYPE_FILE_ID,
    SAHPI_IDR_FIELDTYPE_ASSET_TAG,
    SAHPI_IDR_FIELDTYPE_CUSTOM,
    SAHPI_IDR_FIELDTYPE_UNSPECIFIED = 0xFF
} SaHpiIdrFieldTypeT;

/* Inventory Data Field structure definition */
typedef struct {
    SaHpiEntryIdT        AreaId;    /* AreaId for the IDA to which */
                                    /* the Field belongs */
    SaHpiEntryIdT        FieldId;   /* Field Identifier */
    SaHpiIdrFieldTypeT   Type;      /* Field Type */
    SaHpiBoolT           ReadOnly;  /* Describes if a field is read-only. */
                                    /* All fields in a read-only area are */
                                    /* flagged as read-only as well.*/
    SaHpiTextBufferT     Field;     /* Field Data */ 
} SaHpiIdrFieldT;

/* Inventory Data Area header structure definition */
typedef struct {
    SaHpiEntryIdT      AreaId;      /* Area Identifier */
    SaHpiIdrAreaTypeT  Type;        /* Type of area */
    SaHpiBoolT         ReadOnly;    /* Describes if an area is read-only. */
                                    /* All area headers in a read-only IDR */
                                    /* are flagged as read-only as well.*/
    SaHpiUint32T       NumFields;   /* Number of Fields contained in Area */
} SaHpiIdrAreaHeaderT;

/* Inventory Data Repository Information structure definition */
typedef struct {
    SaHpiIdrIdT        IdrId;       /* Repository Identifier */ 
    SaHpiUint32T       UpdateCount; /* The count is incremented any time the */
                                    /* IDR is changed.  It rolls over to zero */
                                    /* when the maximum value is reached */
    SaHpiBoolT         ReadOnly;    /* Describes if the IDR is read-only. */
                                    /* All area headers and fields in a  */
                                    /* read-only IDR are flagged as */
                                    /* read-only as well.*/
    SaHpiUint32T       NumAreas;    /* Number of Area contained in IDR */
} SaHpiIdrInfoT;


/*******************************************************************************
********************************************************************************
**********                                                            **********
**********       Inventory Data Repository Resource Data Records      **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** All inventory data contained in an inventory data repository
** must be represented in the RDR repository
** with an SaHpiInventoryRecT.
*/
typedef struct {
    SaHpiIdrIdT   IdrId;
    SaHpiBoolT    Persistent;  /* True indicates that updates to IDR are
                                  automatically and immediately persisted.
                                  False indicates that updates are not
                                  immediately persisted; but optionally may be
                                  persisted via saHpiParmControl() function, as
                                  defined in implementation documentation.*/
    SaHpiUint32T  Oem;
} SaHpiInventoryRecT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                          Watchdogs                         **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** This section defines all of the data types associated with watchdog timers.
*/

/* Watchdog Number - Identifier for a watchdog timer. */
typedef SaHpiInstrumentIdT SaHpiWatchdogNumT;
#define SAHPI_DEFAULT_WATCHDOG_NUM (SaHpiWatchdogNumT)0x00000000

/*
** Watchdog Timer Action
**
** These enumerations represent the possible actions to be taken upon watchdog 
** timer timeout and the events that are generated for watchdog actions. 
*/
typedef enum { 
    SAHPI_WA_NO_ACTION = 0,
    SAHPI_WA_RESET,
    SAHPI_WA_POWER_DOWN,
    SAHPI_WA_POWER_CYCLE
} SaHpiWatchdogActionT;

typedef enum {
    SAHPI_WAE_NO_ACTION = 0,
    SAHPI_WAE_RESET,
    SAHPI_WAE_POWER_DOWN,
    SAHPI_WAE_POWER_CYCLE,
    SAHPI_WAE_TIMER_INT=0x08   /* Used if Timer Preinterrupt only */
} SaHpiWatchdogActionEventT;

/*
** Watchdog Pre-timer Interrupt
**
** These enumerations represent the possible types of interrupts that may be 
** triggered by a watchdog pre-timer event. The actual meaning of these 
** operations may differ depending on the hardware architecture.
*/
typedef enum { 
    SAHPI_WPI_NONE = 0,
    SAHPI_WPI_SMI,
    SAHPI_WPI_NMI,
    SAHPI_WPI_MESSAGE_INTERRUPT,
    SAHPI_WPI_OEM = 0x0F
} SaHpiWatchdogPretimerInterruptT;

/*
** Watchdog Timer Use 
**
** These enumerations represent the possible watchdog users that may have caused
** the watchdog to expire. For instance, if watchdog is being used during power 
** on self test (POST), and it expires, the SAHPI_WTU_BIOS_POST expiration type 
** will be set. Most specific uses for Watchdog timer by users of HPI should 
** indicate SAHPI_WTU_SMS_OS  if the use is to provide an OS-healthy heartbeat, 
** or SAHPI_WTU_OEM if it is used for some other purpose.
*/
typedef enum { 
    SAHPI_WTU_NONE = 0,
    SAHPI_WTU_BIOS_FRB2,
    SAHPI_WTU_BIOS_POST,
    SAHPI_WTU_OS_LOAD,
    SAHPI_WTU_SMS_OS,            /* System Management System providing 
                                   heartbeat for OS */
    SAHPI_WTU_OEM,
    SAHPI_WTU_UNSPECIFIED = 0x0F
} SaHpiWatchdogTimerUseT;

/*
** Timer Use Expiration Flags
** These values are used for the Watchdog Timer Use Expiration flags in the 
** SaHpiWatchdogT structure.
*/
typedef SaHpiUint8T SaHpiWatchdogExpFlagsT;
#define SAHPI_WATCHDOG_EXP_BIOS_FRB2   (SaHpiWatchdogExpFlagsT)0x02
#define SAHPI_WATCHDOG_EXP_BIOS_POST   (SaHpiWatchdogExpFlagsT)0x04
#define SAHPI_WATCHDOG_EXP_OS_LOAD     (SaHpiWatchdogExpFlagsT)0x08
#define SAHPI_WATCHDOG_EXP_SMS_OS      (SaHpiWatchdogExpFlagsT)0x10
#define SAHPI_WATCHDOG_EXP_OEM         (SaHpiWatchdogExpFlagsT)0x20

/*
** Watchdog Structure
** 
** This structure is used by the saHpiWatchdogTimerGet() and 
** saHpiWatchdogTimerSet() functions. The use of the structure varies slightly
** by each function.
**
** For saHpiWatchdogTimerGet() :
**
**   Log -                indicates whether or not the Watchdog is configured to
**                        issue events. True=events will be generated.
**   Running -            indicates whether or not the Watchdog is currently 
**                        running or stopped. True=Watchdog is running.
**   TimerUse -           indicates the current use of the timer; one of the  
**                        enumerated preset uses which was included on the last
**                        saHpiWatchdogTimerSet() function call, or through some
**                        other implementation-dependent means to start the 
**                        Watchdog timer.
**   TimerAction -        indicates what action will be taken when the Watchdog
**                        times out.
**   PretimerInterrupt -  indicates which action will be taken 
**                        "PreTimeoutInterval" milliseconds prior to Watchdog 
**                        timer expiration. 
**   PreTimeoutInterval - indicates how many milliseconds prior to timer time
**                        out the PretimerInterrupt action will be taken. If 
**                        "PreTimeoutInterval" = 0, the PretimerInterrupt action
**                        will occur concurrently with "TimerAction." HPI 
**                        implementations may not be able to support millisecond
**                        resolution, and because of this may have rounded the 
**                        set value to whatever resolution could be supported.  
**                        The HPI implementation will return this rounded value.
**   TimerUseExpFlags -   set of five bit flags which indicate that a Watchdog
**                        timer timeout has occurred while the corresponding 
**                        TimerUse value was set. Once set, these flags stay 
**                        set until specifically cleared with a 
**                        saHpiWatchdogTimerSet() call, or by some other 
**                        implementation-dependent means.
**   InitialCount -       The time, in milliseconds, before the timer will time
**                        out after the watchdog is started/restarted, or some 
**                        other implementation-dependent strobe is
**                        sent to the Watchdog. HPI implementations may not be 
**                        able to support millisecond resolution, and because 
**                        of this may have rounded the set value to whatever 
**                        resolution could be supported.  The HPI implementation
**                        will return this rounded value.
**   PresentCount -       The remaining time in milliseconds before the timer 
**                        will time out unless a saHpiWatchdogTimerReset()
**                        function call is made, or some other implementation-
**                        dependent strobe is sent to the Watchdog. 
**                        HPI implementations may not be able to support 
**                        millisecond resolution on watchdog timers, but will 
**                        return the number of clock ticks remaining times the 
**                        number of milliseconds between each tick.
**
** For saHpiWatchdogTimerSet():
**
**   Log -                indicates whether or not the Watchdog should issue 
**                        events. True=event will be generated.
**   Running -            indicates whether or not the Watchdog should be 
**                        stopped before updating. 
**                        True =  Watchdog is not stopped. If it is 
**                                already stopped, it will remain stopped,
**                                but if it is running, it will continue
**                                to run, with the countown timer reset
**                                to the new InitialCount.  Note that 
**                                there is a race condition possible 
**                                with this setting, so it should be used
**                                with care. 
**                        False = Watchdog is stopped. After 
**                                saHpiWatchdogTimerSet() is called, a 
**                                subsequent call to 
**                                saHpiWatchdogTimerReset() is required 
**                                to start the timer.
**   TimerUse -           indicates the current use of the timer. Will control 
**                        which TimerUseExpFlag is set if the timer expires.
**   TimerAction -        indicates what action will be taken when the Watchdog
**                        times out.
**   PretimerInterrupt -  indicates which action will be taken 
**                        "PreTimeoutInterval" milliseconds prior to Watchdog 
**                        timer expiration. 
**   PreTimeoutInterval - indicates how many milliseconds prior to timer time 
**                        out the PretimerInterrupt action will be taken. If 
**                        "PreTimeoutInterval" = 0, the PretimerInterrupt action
**                        will occur concurrently with "TimerAction." HPI 
**                        implementations may not be able to support millisecond
**                        resolution and may have a maximum value restriction. 
**                        These restrictions should be documented by the 
**                        provider of the HPI interface.
**   TimerUseExpFlags -   Set of five bit flags corresponding to the five 
**                        TimerUse values. For each bit set, the corresponding 
**                        Timer Use Expiration Flag will be CLEARED. Generally, 
**                        a program should only clear the Timer Use Expiration 
**                        Flag corresponding to its own TimerUse, so that other 
**                        software, which may have used the timer for another 
**                        purpose in the past can still read its TimerUseExpFlag
**                        to determine whether or not the timer expired during 
**                        that use.
**   InitialCount -       The time, in milliseconds, before the timer will time 
**                        out after a saHpiWatchdogTimerReset() function call is
**                        made, or some other implementation-dependent strobe is
**                        sent to the Watchdog. HPI implementations may not be
**                        able to support millisecond resolution and may have a 
**                        maximum value restriction. These restrictions should 
**                        be documented by the provider of the HPI interface.
**   PresentCount -       Not used on saHpiWatchdogTimerSet() function. Ignored.
**
*/

typedef struct {
    SaHpiBoolT                        Log;
    SaHpiBoolT                        Running;
    SaHpiWatchdogTimerUseT            TimerUse;
    SaHpiWatchdogActionT              TimerAction;
    SaHpiWatchdogPretimerInterruptT   PretimerInterrupt;
    SaHpiUint32T                      PreTimeoutInterval;
    SaHpiWatchdogExpFlagsT            TimerUseExpFlags;
    SaHpiUint32T                      InitialCount;
    SaHpiUint32T                      PresentCount;
} SaHpiWatchdogT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                  Watchdog Resource Data Records            **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** When the "Watchdog" capability is set in a resource, a watchdog with an 
** identifier of SAHPI_DEFAULT_WATCHDOG_NUM is required. All watchdogs must be 
** represented in the RDR repository with an SaHpiWatchdogRecT, including the
** watchdog with an identifier of SAHPI_DEFAULT_WATCHDOG_NUM.
*/
typedef struct {
    SaHpiWatchdogNumT  WatchdogNum;
    SaHpiUint32T       Oem;
} SaHpiWatchdogRecT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                           Hot Swap                         **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/* Hot Swap Indicator State */
typedef enum {
    SAHPI_HS_INDICATOR_OFF = 0,
    SAHPI_HS_INDICATOR_ON
} SaHpiHsIndicatorStateT;

/* Hot Swap Action  */
typedef enum {
    SAHPI_HS_ACTION_INSERTION = 0,
    SAHPI_HS_ACTION_EXTRACTION
} SaHpiHsActionT;

/* Hot Swap State */
typedef enum {
    SAHPI_HS_STATE_INACTIVE = 0,
    SAHPI_HS_STATE_INSERTION_PENDING,
    SAHPI_HS_STATE_ACTIVE,
    SAHPI_HS_STATE_EXTRACTION_PENDING,
    SAHPI_HS_STATE_NOT_PRESENT
} SaHpiHsStateT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                        Events, Part 2                      **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/* Event Data Structures */

typedef enum {
    SAHPI_CRITICAL = 0,
    SAHPI_MAJOR,
    SAHPI_MINOR,
    SAHPI_INFORMATIONAL,
    SAHPI_OK,
    SAHPI_DEBUG = 0xF0,
    SAHPI_ALL_SEVERITIES = 0xFF  /* Only used with DAT and Annunciator */
                                 /* functions.  This is not a valid */
                                 /* severity for events or alarms */
} SaHpiSeverityT;


typedef enum {
    SAHPI_RESE_RESOURCE_FAILURE,
    SAHPI_RESE_RESOURCE_RESTORED,
    SAHPI_RESE_RESOURCE_ADDED
} SaHpiResourceEventTypeT;


typedef struct {
    SaHpiResourceEventTypeT  ResourceEventType;   
} SaHpiResourceEventT;


/*
** Domain events are used to announce the addition of domain references 
** and the removal of domain references to the DRT.  
*/
typedef enum {
    SAHPI_DOMAIN_REF_ADDED,
    SAHPI_DOMAIN_REF_REMOVED
} SaHpiDomainEventTypeT;

typedef struct {
    SaHpiDomainEventTypeT  Type;        /* Type of domain event */
    SaHpiDomainIdT         DomainId;    /* Domain Id of domain added
                                           to or removed from DRT. */
} SaHpiDomainEventT;


/*
** Sensor Optional Data
**
** Sensor events may contain optional data items passed and stored with the 
** event. If these optional data items are present, they will be included with 
** the event data returned in response to a saHpiEventGet() or 
** saHpiEventLogEntryGet() function call. Also, the optional data items may be 
** included with the event data passed to the saHpiEventLogEntryAdd() function.
**
** Specific implementations of HPI may have restrictions on how much data may
** be passed to saHpiEventLogEntryAdd(). These restrictions should be documented
** by the provider of the HPI interface.
*/
typedef SaHpiUint8T SaHpiSensorOptionalDataT;

#define SAHPI_SOD_TRIGGER_READING   (SaHpiSensorOptionalDataT)0x01
#define SAHPI_SOD_TRIGGER_THRESHOLD (SaHpiSensorOptionalDataT)0x02
#define SAHPI_SOD_OEM               (SaHpiSensorOptionalDataT)0x04
#define SAHPI_SOD_PREVIOUS_STATE    (SaHpiSensorOptionalDataT)0x08
#define SAHPI_SOD_CURRENT_STATE     (SaHpiSensorOptionalDataT)0x10
#define SAHPI_SOD_SENSOR_SPECIFIC   (SaHpiSensorOptionalDataT)0x20
typedef struct {
    SaHpiSensorNumT           SensorNum;
    SaHpiSensorTypeT          SensorType;
    SaHpiEventCategoryT       EventCategory;
    SaHpiBoolT                Assertion;        /* True = Event State 
                                                   asserted
                                                   False = deasserted */
    SaHpiEventStateT          EventState;       /* single state being asserted 
                                                   or deasserted*/
    SaHpiSensorOptionalDataT  OptionalDataPresent;
    /* the following fields are only valid if the corresponding flag is set
       in the OptionalDataPresent field */
    SaHpiSensorReadingT       TriggerReading;   /* Reading that triggered
                                                   the event */ 
    SaHpiSensorReadingT       TriggerThreshold; /* Value of the threshold
                                                   that was crossed.  Will not
                                                   be present if threshold is
                                                   not readable. */
    SaHpiEventStateT          PreviousState;    /* Previous set of asserted
                                                   event states.  If multiple
                                                   event states change at once,
                                                   multiple events may be
                                                   generated for each changing
                                                   event state.  This field 
                                                   should indicate the status of
                                                   the sensor event states prior
                                                   to any of the simultaneous 
                                                   changes.
            
                                                   Thus, it will be the same in
                                                   each event generated due to
                                                   multiple simultaneous event 
                                                   state changes. */
                                               
    SaHpiEventStateT          CurrentState;     /* Current set of asserted
                                                   event states. */
    SaHpiUint32T              Oem;
    SaHpiUint32T              SensorSpecific;
} SaHpiSensorEventT;

typedef SaHpiUint8T SaHpiSensorEnableOptDataT;

#define SAHPI_SEOD_CURRENT_STATE        (SaHpiSensorEnableOptDataT)0x10

typedef struct {
    SaHpiSensorNumT           SensorNum;
    SaHpiSensorTypeT          SensorType;
    SaHpiEventCategoryT       EventCategory;
    SaHpiBoolT                SensorEnable;  /* current sensor enable status  */
    SaHpiBoolT                SensorEventEnable; /* current evt enable status */
    SaHpiEventStateT          AssertEventMask;   /* current assert event mask */
    SaHpiEventStateT          DeassertEventMask; /* current deassert evt mask */
    SaHpiSensorEnableOptDataT OptionalDataPresent;
    /* the following fields are only valid if the corresponding flag is set
       in the OptionalDataPresent field */
    SaHpiEventStateT          CurrentState;      /* Current set of asserted
                                                    Event states. */
} SaHpiSensorEnableChangeEventT;

typedef struct {
    SaHpiHsStateT HotSwapState;
    SaHpiHsStateT PreviousHotSwapState;
} SaHpiHotSwapEventT;

typedef struct {
    SaHpiWatchdogNumT               WatchdogNum;
    SaHpiWatchdogActionEventT       WatchdogAction;
    SaHpiWatchdogPretimerInterruptT WatchdogPreTimerAction;
    SaHpiWatchdogTimerUseT          WatchdogUse;
} SaHpiWatchdogEventT;

/*
** The following type defines the types of events that can be reported
** by the HPI software implementation. 
**
** Audit events report a discrepancy in the audit process.  Audits are typically
** performed by HA software to detect problems.  Audits may look for such things
** as corrupted data stores, inconsistent RPT information, or improperly managed
** queues.
**
** Startup events report a failure to start-up properly, or inconsistencies in
** persisted data.
*/
typedef enum {
    SAHPI_HPIE_AUDIT,
    SAHPI_HPIE_STARTUP,
    SAHPI_HPIE_OTHER
} SaHpiSwEventTypeT;

typedef struct {
    SaHpiManufacturerIdT MId;
    SaHpiSwEventTypeT   Type;
    SaHpiTextBufferT    EventData;
} SaHpiHpiSwEventT;

typedef struct {
    SaHpiManufacturerIdT MId;
    SaHpiTextBufferT     OemEventData;
} SaHpiOemEventT;

/*
** User events may be used for storing custom events created by an HPI User
** when injecting events into the Event Log using saHpiEventLogEntryAdd().
*/
typedef struct {
    SaHpiTextBufferT     UserEventData;
} SaHpiUserEventT;

typedef enum {
    SAHPI_ET_RESOURCE,
    SAHPI_ET_DOMAIN,
    SAHPI_ET_SENSOR,
    SAHPI_ET_SENSOR_ENABLE_CHANGE,
    SAHPI_ET_HOTSWAP,
    SAHPI_ET_WATCHDOG,
    SAHPI_ET_HPI_SW,
    SAHPI_ET_OEM,
    SAHPI_ET_USER
} SaHpiEventTypeT;

typedef union {
    SaHpiResourceEventT           ResourceEvent;
    SaHpiDomainEventT             DomainEvent;
    SaHpiSensorEventT             SensorEvent;
    SaHpiSensorEnableChangeEventT SensorEnableChangeEvent;
    SaHpiHotSwapEventT            HotSwapEvent;
    SaHpiWatchdogEventT           WatchdogEvent;
    SaHpiHpiSwEventT              HpiSwEvent;
    SaHpiOemEventT                OemEvent;
    SaHpiUserEventT               UserEvent;
} SaHpiEventUnionT;

typedef struct { 
    SaHpiResourceIdT  Source;
    SaHpiEventTypeT   EventType;
    SaHpiTimeT        Timestamp; /*Equal to SAHPI_TIME_UNSPECIFED if time is
                                   not available; Absolute time if greater
                                   than SAHPI_TIME_MAX_RELATIVE, Relative
                                   time if less than or equal to 
                                   SAHPI_TIME_MAX_RELATIVE */
    SaHpiSeverityT    Severity;
    SaHpiEventUnionT  EventDataUnion;
} SaHpiEventT;

/*
**  Event Queue Status
**
**      This status word is returned to HPI Users that request it 
**      when saHpiEventGet() is called.
**
*/

typedef SaHpiUint32T    SaHpiEvtQueueStatusT;

#define SAHPI_EVT_QUEUE_OVERFLOW (SaHpiEvtQueueStatusT)0x0001

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                        Annunciators                        **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** Annunciator Number
** Identifier for an Annunciator management instrument.
*/
typedef SaHpiInstrumentIdT SaHpiAnnunciatorNumT;

/*
** The following data type is equivalent to the AIS data type SaNameT.
** it is defined in this header file, so that inclusion of the AIS
** header file is not required.  This data type is based on version 1.0
** of the AIS specification
*/
#define SA_HPI_MAX_NAME_LENGTH 256

typedef struct {
    SaHpiUint16T  Length;
    unsigned char Value[SA_HPI_MAX_NAME_LENGTH];
} SaHpiNameT;

/*
** Enumeration of Announcement Types 
*/
typedef enum {
    SAHPI_STATUS_COND_TYPE_SENSOR,
    SAHPI_STATUS_COND_TYPE_RESOURCE,
    SAHPI_STATUS_COND_TYPE_OEM,
    SAHPI_STATUS_COND_TYPE_USER
} SaHpiStatusCondTypeT;


/* Condition structure definition */
typedef struct {

    SaHpiStatusCondTypeT Type;         /* Status Condition Type */
    SaHpiEntityPathT     Entity;       /* Entity assoc. with status condition */
    SaHpiDomainIdT       DomainId;     /* Domain associated with status. 
                                          May be SAHPI_UNSPECIFIED_DOMAIND_ID
                                          meaning current domain, or domain
                                          not meaningful for status condition*/
    SaHpiResourceIdT     ResourceId;   /* Resource associated with status.
                                          May be SAHPI_UNSPECIFIED_RESOURCE_ID
                                          if Type is SAHPI_STATUS_COND_USER.
                                          Must be set to valid ResourceId in
                                          domain specified by DomainId,
                                          or in current domain, if DomainId
                                          is SAHPI_UNSPECIFIED_DOMAIN_ID */
    SaHpiSensorNumT      SensorNum;    /* Sensor associated with status 
                                          Only valid if Type is
                                          SAHPI_STATUS_COND_TYPE_SENSOR */
    SaHpiEventStateT     EventState;   /* Sensor event state. 
                                          Only valid if Type is 
                                          SAHPI_STATUS_COND_TYPE_SENSOR. */
    SaHpiNameT           Name;         /* AIS compatible identifier associated
                                          with Status condition */
    SaHpiManufacturerIdT Mid;          /* Manufacturer Id associated with
                                          status condition, required when type
                                          is SAHPI_STATUS_COND_TYPE_OEM. */
    SaHpiTextBufferT     Data;         /* Optional Data associated with
                                          Status condition */
} SaHpiConditionT;


/* Announcement structure definition */
typedef struct {
    SaHpiEntryIdT        EntryId;      /* Announcment Entry Id */
    SaHpiTimeT           Timestamp;    /* Time when announcement added to set */
    SaHpiBoolT           AddedByUser;  /* True if added to set by HPI User,
                                          False if added automatically by 
                                          HPI implementation */
    SaHpiSeverityT       Severity;     /* Severity of announcement */
    SaHpiBoolT           Acknowledged; /* Acknowledged flag */
    SaHpiConditionT      StatusCond;   /* Detailed status condition */
} SaHpiAnnouncementT;


/* Annunciator Mode - defines who may add or delete entries in set.  */

typedef enum {
    SAHPI_ANNUNCIATOR_MODE_AUTO,
    SAHPI_ANNUNCIATOR_MODE_USER,
    SAHPI_ANNUNCIATOR_MODE_SHARED
} SaHpiAnnunciatorModeT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********              Annunciator Resource Data Records             **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** The following enumerated type defines the possible output types 
** which can be associated with an Annunciator Management Instrument 
*/
typedef enum {
    SAHPI_ANNUNCIATOR_TYPE_LED = 0,
    SAHPI_ANNUNCIATOR_TYPE_DRY_CONTACT_CLOSURE,
    SAHPI_ANNUNCIATOR_TYPE_AUDIBLE,
    SAHPI_ANNUNCIATOR_TYPE_LCD_DISPLAY,
    SAHPI_ANNUNCIATOR_TYPE_MESSAGE,
    SAHPI_ANNUNCIATOR_TYPE_COMPOSITE,
    SAHPI_ANNUNCIATOR_TYPE_OEM
} SaHpiAnnunciatorTypeT;


/*
** All annunciator management instruments
** must be represented in the RDR repository
** with an SaHpiAnnunciatorRecT.
*/
typedef struct {
    SaHpiAnnunciatorNumT      AnnunciatorNum;
    SaHpiAnnunciatorTypeT     AnnunciatorType; /* Annunciator Output Type */
    SaHpiBoolT                ModeReadOnly;    /* if True, Mode may 
                                                  not be changed by HPI User */
    SaHpiUint32T              MaxConditions;   /* maximum number of conditions
                                                  that can be held in current 
                                                  set.  0 means no fixed 
                                                  limit.  */
    SaHpiUint32T              Oem;
} SaHpiAnnunciatorRecT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                     Resource Data Record                   **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*
** The following describes the different types of records that exist within a  
** RDR repository and the RDR super-structure to all of the specific RDR types 
** (sensor, inventory data, watchdog, etc.).
*/
typedef enum {
    SAHPI_NO_RECORD,
    SAHPI_CTRL_RDR,
    SAHPI_SENSOR_RDR,
    SAHPI_INVENTORY_RDR,
    SAHPI_WATCHDOG_RDR,
    SAHPI_ANNUNCIATOR_RDR
} SaHpiRdrTypeT;

typedef union {
    SaHpiCtrlRecT        CtrlRec;
    SaHpiSensorRecT      SensorRec;
    SaHpiInventoryRecT   InventoryRec;
    SaHpiWatchdogRecT    WatchdogRec;
    SaHpiAnnunciatorRecT AnnunciatorRec;
} SaHpiRdrTypeUnionT;

typedef struct {
    SaHpiEntryIdT        RecordId;
    SaHpiRdrTypeT        RdrType;
    SaHpiEntityPathT     Entity;        /* Entity to which this RDR relates. */
    SaHpiBoolT           IsFru;         /* Entity is a FRU.  This field is 
                                           Only valid if the Entity path given
                                           in the "Entity" field is different
                                           from the Entity path in the RPT
                                           entry for the resource. */
    SaHpiRdrTypeUnionT   RdrTypeUnion;
    SaHpiTextBufferT     IdString;
} SaHpiRdrT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                      Parameter Control                     **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

typedef enum { 
    SAHPI_DEFAULT_PARM = 0, 
    SAHPI_SAVE_PARM, 
    SAHPI_RESTORE_PARM
} SaHpiParmActionT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                      Reset                                 **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

typedef enum { 
    SAHPI_COLD_RESET = 0, 
    SAHPI_WARM_RESET, 
    SAHPI_RESET_ASSERT,
    SAHPI_RESET_DEASSERT
} SaHpiResetActionT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                      Power                                 **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

typedef enum {
    SAHPI_POWER_OFF = 0,
    SAHPI_POWER_ON,
    SAHPI_POWER_CYCLE
} SaHpiPowerStateT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                    Resource Presence Table                 **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*  This section defines the types associated with the RPT. */

/*
** GUID - Globally Unique Identifier
** 
** The format if the ID follows that specified by the Wired for Management 
** Baseline, Version 2.0 specification.  HPI uses version 1 of the GUID
** format, with a 3-bit variant field of 10x (where x indicates 'don't care')
*/
typedef SaHpiUint8T    SaHpiGuidT[16];

/* 
** Resource Info Type Definitions
** 
** 
** SaHpiResourceInfoT contains static configuration data concerning the 
** management controller associated with the resource, or the resource itself. 
** Note this information is used to describe the resource; that is, the piece of
** infrastructure which manages an entity (or multiple entities) - NOT the 
** entities for which the resource provides management. The purpose of the 
** SaHpiResourceInfoT structure is to provide information that an HPI User may 
** need in order to interact correctly with the resource (e.g., recognize a 
** specific management controller which may have defined OEM fields in sensors,
** OEM controls, etc.).
**
** The GUID is used to uniquely identify a Resource. A GUID value of zero is not
** valid and indicates that the Resource does not have an associated GUID.
**
** All of the fields in the following structure may or may not be used by a 
** given resource.
*/
typedef struct {
    SaHpiUint8T            ResourceRev;
    SaHpiUint8T            SpecificVer;
    SaHpiUint8T            DeviceSupport;
    SaHpiManufacturerIdT   ManufacturerId;
    SaHpiUint16T           ProductId;
    SaHpiUint8T            FirmwareMajorRev;
    SaHpiUint8T            FirmwareMinorRev;
    SaHpiUint8T            AuxFirmwareRev;
    SaHpiGuidT             Guid;
} SaHpiResourceInfoT;

/*
** Resource Capabilities
**
** This definition defines the capabilities of a given resource. One resource 
** may support any number of capabilities using the bit mask. Because each entry
** in an RPT will have the SAHPI_CAPABILITY_RESOURCE bit set, zero is not a
** valid value for the capability flag, and is thus used to indicate "no RPT
** entry present" in some function calls.
**
** SAHPI_CAPABILITY_RESOURCE         
** SAHPI_CAPABILITY_EVT_DEASSERTS
**   Indicates that all sensors on the resource have the property that their
**   Assertion and Deassertion event enable flags are the same. That is,
**   for all event states whose assertion triggers an event, it is
**   guaranteed that the deassertion of that event will also
**   trigger an event. Thus, an HPI User may track the state of sensors on the
**   resource by monitoring events rather than polling for state changes.
** SAHPI_CAPABILITY_AGGREGATE_STATUS 
** SAHPI_CAPABILITY_CONFIGURATION    
** SAHPI_CAPABILITY_MANAGED_HOTSWAP  
**   Indicates that the resource supports the full managed hot swap model. 
**   Since hot swap only makes sense for field-replaceable units, the 
**   SAHPI_CAPABILITY_FRU capability bit must also be set for this resource.
** SAHPI_CAPABILITY_WATCHDOG         
** SAHPI_CAPABILITY_CONTROL          
** SAHPI_CAPABILITY_FRU
**   Indicates that the resource is a field-replaceable unit; i.e., it is
**   capable of being removed and replaced in a live system. If
**   SAHPI_CAPABILITY_MANAGED_HOTSWAP is also set, the resource supports
**   the full hot swap model.  If SAHPI_CAPABILITY_MANAGED_HOTSWAP is not
**   set, the resource supports the simplified hot swap model.
** SAHPI_CAPABILITY_ANNUNCIATOR
** SAHPI_CAPABILITY_POWER
** SAHPI_CAPABILITY_RESET
** SAHPI_CAPABILITY_INVENTORY_DATA   
** SAHPI_CAPABILITY_EVENT_LOG              
** SAHPI_CAPABILITY_RDR 
**   Indicates that a resource data record (RDR) repository is supplied
**   by the resource. Since the existence of an RDR is mandatory for all
**   management instruments, this
**   capability must be asserted if the resource
**   contains any sensors, controls, watchdog timers, or inventory
**   data repositories.         
** SAHPI_CAPABILITY_SENSOR           
*/

typedef SaHpiUint32T SaHpiCapabilitiesT;
#define SAHPI_CAPABILITY_RESOURCE         (SaHpiCapabilitiesT)0X40000000
#define SAHPI_CAPABILITY_EVT_DEASSERTS    (SaHpiCapabilitiesT)0x00008000
#define SAHPI_CAPABILITY_AGGREGATE_STATUS (SaHpiCapabilitiesT)0x00002000
#define SAHPI_CAPABILITY_CONFIGURATION    (SaHpiCapabilitiesT)0x00001000
#define SAHPI_CAPABILITY_MANAGED_HOTSWAP  (SaHpiCapabilitiesT)0x00000800
#define SAHPI_CAPABILITY_WATCHDOG         (SaHpiCapabilitiesT)0x00000400
#define SAHPI_CAPABILITY_CONTROL          (SaHpiCapabilitiesT)0x00000200
#define SAHPI_CAPABILITY_FRU              (SaHpiCapabilitiesT)0x00000100
#define SAHPI_CAPABILITY_ANNUNCIATOR      (SaHpiCapabilitiesT)0x00000040
#define SAHPI_CAPABILITY_POWER            (SaHpiCapabilitiesT)0x00000020
#define SAHPI_CAPABILITY_RESET            (SaHpiCapabilitiesT)0x00000010
#define SAHPI_CAPABILITY_INVENTORY_DATA   (SaHpiCapabilitiesT)0x00000008
#define SAHPI_CAPABILITY_EVENT_LOG        (SaHpiCapabilitiesT)0x00000004
#define SAHPI_CAPABILITY_RDR              (SaHpiCapabilitiesT)0x00000002
#define SAHPI_CAPABILITY_SENSOR           (SaHpiCapabilitiesT)0x00000001

/*
** Resource Managed Hot Swap Capabilities
**
** This definition defines the managed hot swap capabilities of a given
** resource. 
**
** SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY
** This capability indicates if the hot swap autoextract timer is read-only.
** SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED
** Indicates whether or not the resource has a hot swap indicator.
*/

typedef SaHpiUint32T SaHpiHsCapabilitiesT;
#define SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY \
   (SaHpiHsCapabilitiesT)0x80000000
#define SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED \
   (SaHpiHsCapabilitiesT)0X40000000
/*
** RPT Entry
**
** This structure is used to store the RPT entry information.
**
** The ResourceCapabilities field definies the capabilities of the resource.
** This field must be non-zero for all valid resources.
** 
** The HotSwapCapabilities field denotes the capabilities of the resource,
** specifically related to hot swap.  This field is only valid if the
** resource supports managed hot swap, as indicated by the 
** SAHPI_CAPABILITY_MANAGED_HOT_SWAP resource capability.
**
** The ResourceTag is a data field within an RPT entry available to the HPI 
** User for associating application specific data with a resource.  The HPI 
** User supplied data is purely informational and is not used by the HPI 
** implementation, domain, or associated resource.  For example, an HPI User 
** can set the resource tag to a "descriptive" value, which can be used to 
** identify the resource in messages to a human operator.
*/
typedef struct {
    SaHpiEntryIdT        EntryId;
    SaHpiResourceIdT     ResourceId;
    SaHpiResourceInfoT   ResourceInfo;
    SaHpiEntityPathT     ResourceEntity;  /* If resource manages a FRU, entity
                                             path of the FRU */
                                          /* If resource manages a single 
                                             entity, entity path of that
                                             entity. */
                                          /* If resource manages multiple
                                             entities, the entity path of the
                                             "primary" entity managed by the
                                             resource */
                                          /* Must be set to the same value in
                                             every domain which contains this
                                             resource */
    SaHpiCapabilitiesT   ResourceCapabilities;  /* Must be non-0. */
    SaHpiHsCapabilitiesT HotSwapCapabilities; /* Indicates the hot swap 
                                                 capabilities of the resource */
    SaHpiSeverityT       ResourceSeverity; /* Indicates the criticality that
                                              should be raised when the resource
                                              is not responding   */
    SaHpiBoolT           ResourceFailed;  /* Indicates that the resource is not
                                             currently functional */
    SaHpiTextBufferT     ResourceTag;
} SaHpiRptEntryT; 

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                      Domain Information                    **********
**********                                                            **********
********************************************************************************
*******************************************************************************/

/*  This section defines the types associated with the domain controller. */

/*
** Domain Capabilities
**
** This definition defines the capabilities of a given domain. A domain 
** may support any number of capabilities using the bit mask. 
**
** SAHPI_DOMAIN_CAP_AUTOINSERT_READ_ONLY
**   Indicates that the domain auto insert timeout value is read-only
**   and may not be modified using the saHpiHotSwapAutoInsertTimeoutSet()
**   function.
*/

typedef SaHpiUint32T SaHpiDomainCapabilitiesT;
#define SAHPI_DOMAIN_CAP_AUTOINSERT_READ_ONLY \
   (SaHpiDomainCapabilitiesT)0X00000001

/*
** Domain Info
**
** This structure is used to store the information regarding the domain 
** including information regarding the domain reference table (DRT) and 
** the resource presence table (RPT).
**
** The DomainTag field is an informational value that supplies an HPI User  
** with naming information for the domain.
**
** NOTE: Regarding timestamps - If the implementation cannot supply an absolute
** timestamp, then it may supply a timestamp relative to some system-defined 
** epoch, such as system boot. The value SAHPI_TIME_UNSPECIFIED indicates that
** the time of the update cannot be determined. Otherwise, If the value is less
** than or equal to SAHPI_TIME_MAX_RELATIVE, then it is relative; if it is
** greater than SAHPI_TIME_MAX_RELATIVE, then it is absolute. 
**
** The GUID is used to uniquely identify a domain. A GUID value of zero is not
** valid and indicates that the domain does not have an associated GUID.
*/

typedef struct {
    SaHpiDomainIdT    DomainId;        /* Unique Domain Id associated with
                                          domain */
    SaHpiDomainCapabilitiesT DomainCapabilities;    /* Domain Capabilities */
    SaHpiBoolT        IsPeer;          /* Indicates that this domain
                                          participates in a peer
                                          relationship. */
    SaHpiTextBufferT  DomainTag;       /* Information tag associated with
                                          domain */
    SaHpiUint32T      DrtUpdateCount;  /* This count is incremented any time the
                                          table is changed. It rolls over to 
                                          zero when the maximum value is
                                          reached  */
    SaHpiTimeT        DrtUpdateTimestamp; /* This timestamp is set any time the
                                             DRT table is changed. */
    SaHpiUint32T      RptUpdateCount;  /* This count is incremented any time 
                                           the RPT is changed. It rolls over
                                           to zero when the maximum value is
                                           reached  */
    SaHpiTimeT        RptUpdateTimestamp; /* This timestamp is set any time the 
                                             RPT table is changed. */
    SaHpiUint32T      DatUpdateCount;  /* This count is incremented any time 
                                          the DAT is changed. It rolls over to 
                                          zero when the maximum value is 
                                          reached */
    SaHpiTimeT        DatUpdateTimestamp; /* This timestamp is set any time the
                                              DAT is changed. */
    SaHpiUint32T      ActiveAlarms;    /* Count of active alarms in the DAT */
    SaHpiUint32T      CriticalAlarms;  /* Count of active critical alarms in
                                          the DAT */
    SaHpiUint32T      MajorAlarms;     /* Count of active major alarms in the
                                          DAT */
    SaHpiUint32T      MinorAlarms;     /* Count of active minor alarms in the
                                          DAT */
    SaHpiUint32T      DatUserAlarmLimit; /* Maximum User Alarms that can be 
                                            added to DAT.  0=no fixed limit */
    SaHpiBoolT        DatOverflow;     /* Set to True if there are one 
                                          or more non-User Alarms that   
                                          are missing from the DAT because 
                                          of space limitations */
    SaHpiGuidT        Guid;            /* GUID associated with domain.*/
} SaHpiDomainInfoT;

/*
** DRT Entry
**
** This structure is used to store the DRT entry information.
**
*/
typedef struct {
    SaHpiEntryIdT        EntryId;
    SaHpiDomainIdT       DomainId;  /* The Domain ID referenced by this entry */
    SaHpiBoolT           IsPeer;    /* Indicates if this domain reference
                                       is a peer. If not, the domain reference
                                       is a tier. */
} SaHpiDrtEntryT; 


/*
** DAT Entry
**
** This structure is used to store alarm informatin in the DAT
**
*/


typedef SaHpiEntryIdT SaHpiAlarmIdT;

typedef struct {
    SaHpiAlarmIdT       AlarmId;      /* Alarm Id */
    SaHpiTimeT          Timestamp;    /* Time when alarm added to DAT */
    SaHpiSeverityT      Severity;     /* Severity of alarm */
    SaHpiBoolT          Acknowledged; /* Acknowledged flag */
    SaHpiConditionT     AlarmCond;    /* Detailed alarm condition */
} SaHpiAlarmT;

/*******************************************************************************
********************************************************************************
**********                                                            **********
**********                       Event Log                            **********
**********                                                            **********
********************************************************************************
*******************************************************************************/
/* This section defines the types associated with the Event Log. */
/* 
** Event Log Information
**
** The Entries entry denotes the number of active entries contained in the Event
**   Log. 
** The Size entry denotes the total number of entries the Event Log is able to 
**   hold. 
** The UserEventMaxSize entry indicates the maximum size of the text buffer
**   data field in an HPI User event that is supported by the Event Log 
**   implementation.  If the implementation does not enforce a more restrictive
**   data length, it should be set to SAHPI_MAX_TEXT_BUFFER_LENGTH.
** The UpdateTimestamp entry denotes the time of the last update to the Event 
**   Log. 
** The CurrentTime entry denotes the Event Log's idea of the current time; i.e 
**   the timestamp that would be placed on an entry if it was added now. 
** The Enabled entry indicates whether the Event Log is enabled. If the Event 
**   Log is "disabled" no events generated within the HPI implementation will be
**   added to the Event Log. Events may still be added to the Event Log with
**   the saHpiEventLogEntryAdd() function. When the Event Log is "enabled"
**   events may be automatically added to the Event Log as they are generated
**   in a resource or a domain, however, it is implementation-specific which
**   events are automatically added to any Event Log.
** The OverflowFlag entry indicates the Event Log has overflowed. Events have  
**   been dropped or overwritten due to a table overflow. 
** The OverflowAction entry indicates the behavior of the Event Log when an  
**   overflow occurs. 
** The OverflowResetable entry indicates if the overflow flag can be
**   cleared by an HPI User with the saHpiEventLogOverflowReset() function.
*/
typedef enum {
    SAHPI_EL_OVERFLOW_DROP,       /* New entries are dropped when Event Log is
                                    full*/
    SAHPI_EL_OVERFLOW_OVERWRITE  /* Event Log overwrites existing entries
                                    when Event Log is full */
} SaHpiEventLogOverflowActionT;

typedef struct {
    SaHpiUint32T                   Entries;        
    SaHpiUint32T                   Size;      
    SaHpiUint32T                   UserEventMaxSize;
    SaHpiTimeT                     UpdateTimestamp;
    SaHpiTimeT                     CurrentTime;
    SaHpiBoolT                     Enabled;
    SaHpiBoolT                     OverflowFlag;
    SaHpiBoolT                     OverflowResetable;
    SaHpiEventLogOverflowActionT   OverflowAction;
} SaHpiEventLogInfoT;
/*
** Event Log Entry
** These types define the Event Log entry.
*/
typedef SaHpiUint32T SaHpiEventLogEntryIdT;
/* Reserved values for Event Log entry IDs */
#define SAHPI_OLDEST_ENTRY    (SaHpiEventLogEntryIdT)0x00000000
#define SAHPI_NEWEST_ENTRY    (SaHpiEventLogEntryIdT)0xFFFFFFFF
#define SAHPI_NO_MORE_ENTRIES (SaHpiEventLogEntryIdT)0xFFFFFFFE

typedef struct {
    SaHpiEventLogEntryIdT EntryId;   /* Entry ID for record */
    SaHpiTimeT            Timestamp; /* Time at which the event was placed
                                   in the Event Log. If less than or equal to
                                   SAHPI_TIME_MAX_RELATIVE, then it is 
                                   relative; if it is greater than SAHPI_TIME_
                                   MAX_RELATIVE, then it is absolute. */
    SaHpiEventT           Event;     /* Logged Event */
} SaHpiEventLogEntryT;


/*******************************************************************************
**
** Name: saHpiVersionGet() 
**
** Description:
**   This function returns the version identifier of the SaHpi specification 
**   version supported by the HPI implementation.
**
** Parameters:
**   None.
**
** Return Value:
**   The interface version identifier, of type SaHpiVersionT is returned. 
**
** Remarks:
**   This function differs from all other interface functions in that it 
**   returns the version identifier rather than a standard return code.  This is
**   because the version itself is necessary in order for an HPI User to 
**   properly interpret subsequent API return codes.  Thus, the 
**   saHpiVersionGet() function returns the interface version identifier 
**   unconditionally.
**  
**   This function returns the value of the header file symbol 
**   SAHPI_INTERFACE_VERSION in the SaHpi.h header file used when the library 
**   was compiled.  An HPI User may compare the returned value to the 
**   SAHPI_INTERFACE_VERSION symbol in the SaHpi.h header file used by the
**   calling program to determine if the accessed library is compatible with the
**   calling program.
**
*******************************************************************************/
SaHpiVersionT SAHPI_API saHpiVersionGet ( void );

/*******************************************************************************
**
** Name: saHpiSessionOpen()
**
** Description:
**   This function opens an HPI session for a given domain and set of security
**   characteristics (future).
**
** Parameters:
**   DomainId - [in] Domain identifier of the domain to be accessed by the HPI
**      User. A domain identifier of SAHPI_UNSPECIFIED_DOMAIN_ID requests that 
**      a session be opened to a default domain. 
**   SessionId - [out] Pointer to a location to store an identifier for the 
**      newly opened session. This identifier is used for subsequent access to
**      domain resources and events.
**   SecurityParams - [in] Pointer to security and permissions data structure. 
**      This parameter is reserved for future use, and must be set to NULL.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_DOMAIN is returned if no domain matching the specified
**      domain identifier exists.
**   SA_ERR_HPI_INVALID_PARAMS is returned if:
**      * A non-null SecurityParams pointer is passed.
**      * The SessionId pointer is passed in as NULL.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if no more sessions can be opened.
**
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSessionOpen (
    SAHPI_IN  SaHpiDomainIdT      DomainId,
    SAHPI_OUT SaHpiSessionIdT     *SessionId,
    SAHPI_IN  void                *SecurityParams
);

/*******************************************************************************
**
** Name: saHpiSessionClose()
**
** Description:
**   This function closes an HPI session. After closing a session, the SessionId
**   will no longer be valid.
**
** Parameters:
**   SessionId - [in] Session identifier previously obtained using 
**      saHpiSessionOpen().
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSessionClose (
    SAHPI_IN SaHpiSessionIdT SessionId
);

/*******************************************************************************
**
** Name: saHpiDiscover()
**
** Description:
**   This function requests the underlying management service to discover
**   information about resources and associated domains.
**
**   This function may be called during operation to update the DRT table and
**   the RPT table. An HPI implementation may exhibit latency between when
**   hardware changes occur and when the domain DRT and RPT are updated.  To
**   overcome this latency, the saHpiDiscover() function may be called.  When
**   this function returns, the DRT and RPT should be updated to reflect the
**   current system configuration and status.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained 
**      using saHpiSessionOpen().
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiDiscover (
    SAHPI_IN SaHpiSessionIdT SessionId  
);

/*******************************************************************************
**
** Name: saHpiDomainInfoGet()
**
** Description:
**   This function is used for requesting information about the domain, the
**   Domain Reference Table (DRT), the Resource Presence Table (RPT), and the
**   Domain Alarm Table (DAT), such as table update counters and timestamps, and
**   the unique domain identifier associated with the domain. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained
**      using saHpiSessionOpen().
**   DomainInfo - [out] Pointer to the information describing the domain and 
**      DRT.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the DomainInfo pointer is passed
**      in as NULL.
**
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiDomainInfoGet (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_OUT SaHpiDomainInfoT     *DomainInfo
); 

/*******************************************************************************
**
** Name: saHpiDrtEntryGet()
**
** Description:
**   This function retrieves domain information for the specified entry of the
**   DRT. This function allows an HPI User to read the DRT entry-by-entry.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   EntryId - [in] Identifier of the DRT entry to retrieve. Reserved EntryId
**      values:
**         * SAHPI_FIRST_ENTRY  Get first entry
**         * SAHPI_LAST_ENTRY   Reserved as delimiter for end of list. Not a
**            valid entry identifier.
**   NextEntryId - [out] Pointer to location to store the EntryId of next entry
**      in DRT.
**   DrtEntry - [out] Pointer to the structure to hold the returned DRT entry.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * Entry identified by EntryId is not present.
**      * EntryId is SAHPI_FIRST_ENTRY and the DRT is empty.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * DrtEntry pointer is passed in as NULL.
**      * NextEntryId pointer is passed in as NULL.
**      * EntryId is an invalid reserved value such as SAHPI_LAST_ENTRY.
**
** Remarks:
**   If the EntryId parameter is set to SAHPI_FIRST_ENTRY, the first entry in
**   the DRT will be returned. When an entry is successfully retrieved, 
**   NextEntryId will be set to the identifier of the next valid entry; however,
**   when the last entry has been retrieved, NextEntryId will be set to 
**   SAHPI_LAST_ENTRY. To retrieve an entire list of entries, call this function
**   first with an EntryId of SAHPI_FIRST_ENTRY and then use the returned 
**   NextEntryId in the next call. Proceed until the NextEntryId returned is
**   SAHPI_LAST_ENTRY.
**
**   If an HPI User has not subscribed to receive events and a DRT entry is 
**   added while the DRT is being read, that new entry may be missed.  The 
**   update counter provides a means for insuring that no domains are missed
**   when stepping through the DRT. In order to use this feature, an HPI User
**   should call saHpiDomainInfoGet() to get the update counter value before
**   retrieving the first DRT entry. After reading the last entry, the HPI User
**   should again call saHpiDomainInfoGet() to get the update counter value. If
**   the update counter has not been incremented, no new entries have been 
**   added. 
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiDrtEntryGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiEntryIdT       EntryId,
    SAHPI_OUT SaHpiEntryIdT       *NextEntryId,
    SAHPI_OUT SaHpiDrtEntryT      *DrtEntry
);

/*******************************************************************************
**
** Name: saHpiDomainTagSet()
**
** Description:
**   This function allows an HPI User to set a descriptive tag for a particular
**   domain. The domain tag is an informational value that supplies an HPI User
**   with naming information for the domain. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   DomainTag - [in] Pointer to SaHpiTextBufferT containing the domain tag.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_PARAMS is returned if the SaHpiTextBufferT structure
**      passed as DomainTag is not valid.  This would occur when:
**         * The DataType is not one of the enumerated values for that type, or
**         * The data field contains characters that are not legal according to
**            the value of DataType, or 
**         * The Language is not one of the enumerated values for that type when
**            the DataType is SAHPI_TL_TYPE_UNICODE or SAHPI_TL_TYPE_TEXT.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the DomainTag pointer is passed in
**      as NULL.
**
** Remarks:
**   Typically, the HPI implementation will provide an appropriate default value
**   for the domain tag; this function is provided so that an HPI User can
**   override the default, if desired. The value of the domain tag may be
**   retrieved from the domain's information structure.
**
**   The domain tag is not necessarily persistent, and may return to its default
**   value if the domain controller function for the domain restarts.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiDomainTagSet (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiTextBufferT     *DomainTag
);

/*******************************************************************************
**
** Name: saHpiRptEntryGet()
**
** Description:
**   This function retrieves resource information for the specified entry of the
**   resource presence table. This function allows an HPI User to read the RPT
**   entry-by-entry.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   EntryId - [in] Identifier of the RPT entry to retrieve. Reserved EntryId
**      values:
**         * SAHPI_FIRST_ENTRY     Get first entry.
**         * SAHPI_LAST_ENTRY      Reserved as delimiter for end of list. Not a
**            valid entry identifier.
**   NextEntryId - [out] Pointer to location to store the EntryId of next entry
**      in RPT.
**   RptEntry - [out] Pointer to the structure to hold the returned RPT entry.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_NOT_PRESENT is returned when the:
**      * Entry identified by EntryId is not present.
**      * EntryId is SAHPI_FIRST_ENTRY and the RPT is empty.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * RptEntry pointer is passed in as NULL.
**      * NextEntryId pointer is passed in as NULL.
**      * EntryId is an invalid reserved value such as SAHPI_LAST_ENTRY.
**
** Remarks:
**   If the EntryId parameter is set to SAHPI_FIRST_ENTRY, the first entry in
**   the RPT will be returned. When an entry is successfully retrieved, 
**   NextEntryId will be set to the identifier of the next valid entry; however,
**   when the last entry has been retrieved, NextEntryId will be set to 
**   SAHPI_LAST_ENTRY. To retrieve an entire list of entries, call this function
**   first with an EntryId of SAHPI_FIRST_ENTRY and then use the returned 
**   NextEntryId in the next call. Proceed until the NextEntryId returned is
**   SAHPI_LAST_ENTRY.
**
**   At initialization, an HPI User may not wish to turn on eventing, since the
**   context of the events, as provided by the RPT, is not known. In this 
**   instance, if a FRU is inserted into the system while the RPT is being read
**   entry by entry, the resource associated with that FRU may be missed. (Keep
**   in mind that there is no specified ordering for the RPT entries.)  The
**   update counter provides a means for insuring that no resources are missed
**   when stepping through the RPT. In order to use this feature, an HPI User
**   should invoke saHpiDomainInfoGet(), and get the update counter value before
**   retrieving the first RPT entry. After reading the last entry, an HPI User
**   should again invoke the saHpiDomainInfoGet() to get the RPT update counter
**   value. If the update counter has not been incremented, no new records have
**   been added. 
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiRptEntryGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiEntryIdT       EntryId,
    SAHPI_OUT SaHpiEntryIdT       *NextEntryId,
    SAHPI_OUT SaHpiRptEntryT      *RptEntry
);

/*******************************************************************************
**
** Name: saHpiRptEntryGetByResourceId()
**
** Description:
**   This function retrieves resource information from the resource presence
**   table for the specified resource using its ResourceId. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained 
**      using saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   RptEntry  - [out] Pointer to structure to hold the returned RPT entry.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_PARAMS is returned if the RptEntry pointer is passed
**      in as NULL.
**
** Remarks:
**   Typically at start-up, the RPT is read entry-by-entry, using
**   saHpiRptEntryGet(). From this, an HPI User can establish the set of
**   ResourceIds to use for future calls to the HPI functions.
**
**   However, there may be other ways of learning ResourceIds without first
**   reading the RPT. For example, resources may be added to the domain while
**   the system is running in response to a hot swap action. When a resource is
**   added, the application will receive a hot swap event containing the
**   ResourceId of the new resource. The application may then want to search the
**   RPT for more detailed information on the newly added resource. In this 
**   case, the ResourceId can be used to locate the applicable RPT entry
**   information.
**
**   Note that saHpiRptEntryGetByResourceId() is valid in any hot swap state, 
**   except for SAHPI_HS_STATE_NOT_PRESENT.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiRptEntryGetByResourceId (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_OUT SaHpiRptEntryT       *RptEntry
);

/*******************************************************************************
**
** Name: saHpiResourceSeveritySet()
**
** Description:
**   This function allows an HPI User to set the severity level applied to an
**   event issued if a resource unexpectedly becomes unavailable to the HPI. A 
**   resource may become unavailable for several reasons including:
**      * The FRU associated with the resource is no longer present in the
**          system (a surprise extraction has occurred.)
**      * A catastrophic failure has occurred.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   Severity - [in] Severity level of event issued when the resource
**      unexpectedly becomes unavailable to the HPI.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_PARAMS is returned when the value for Severity is not
**      one of the valid enumerated values for this type.
**
** Remarks:
**   Typically, the HPI implementation will provide an appropriate default value
**   for the resource severity, which may vary by resource; an HPI User can
**   override this default value by use of this function.
**
**   If a resource is removed from, then re-added to the RPT (e.g., because of a
**   hot swap action), the HPI implementation may reset the value of the
**   resource severity.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceSeveritySet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId,
    SAHPI_IN  SaHpiSeverityT      Severity
);

/*******************************************************************************
**
** Name: saHpiResourceTagSet()
**
** Description:
**   This function allows an HPI User to set the resource tag of an RPT entry
**   for a particular resource. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen(). 
**   ResourceId - [in] Resource identified for this operation.
**   ResourceTag - [in] Pointer to SaHpiTextBufferT containing the resource tag.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is 
**      returned. 
**   SA_ERR_HPI_INVALID_PARAMS is returned if the SaHpiTextBufferT structure
**      passed as ResourceTag is not valid.  This would occur when:
**      * The DataType is not one of the enumerated values for that type, or 
**      * The data field contains characters that are not legal according to the
**         value of DataType, or
**      * The Language is not one of the enumerated values for that type when
**         the DataType is SAHPI_TL_TYPE_UNICODE or SAHPI_TL_TYPE_TEXT.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the ResourceTag pointer is passed
**      in as NULL.
**
** Remarks:
**   The resource tag is a data field within an RPT entry available to an HPI 
**   User for associating application specific data with a resource.  HPI User
**   supplied data is purely informational and is not used by the HPI 
**   implementation, domain, or associated resource.  For example, an HPI User
**   can set the resource tag to a "descriptive" value, which can be used to
**   identify the resource in messages to a human operator.
**
**   Since the resource tag is contained within an RPT entry, its scope is
**   limited to a single domain.  A resource that exists in more than one domain
**   will have independent resource tags within each domain.
**
**   Typically, the HPI implementation will provide an appropriate default value
**   for the resource tag; this function is provided so that an HPI User can
**   override the default, if desired. The value of the resource tag may be
**   retrieved from the resource's RPT entry.
**
**   If a resource is removed from, then re-added to the RPT (e.g., because of a
**   hot swap action), the HPI implementation may reset the value of the
**   resource tag.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceTagSet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId,
    SAHPI_IN  SaHpiTextBufferT    *ResourceTag
);

/*******************************************************************************
**
** Name: saHpiResourceIdGet()
**
** Description:
**   This function returns the ResourceId of the resource associated with the
**   entity upon which the HPI User is running.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [out] Pointer to location to hold the returned ResourceId. 
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the ResourceId pointer is passed
**      in as NULL.
**   SA_ERR_HPI_NOT_PRESENT is returned if the entity the HPI User is running on
**      is not manageable in the addressed domain.
**   SA_ERR_HPI_UNKNOWN is returned if the domain controller cannot determine an
**      appropriate response. That is, there may be an appropriate ResourceId in
**      the domain to return, but it cannot be determined.
**
** Remarks:
**   This function must be issued within a session to a domain that includes a
**   resource associated with the entity upon which the HPI User is running, or
**   the SA_ERR_HPI_NOT_PRESENT return will be issued. 
**
**   Since entities are contained within other entities, there may be multiple
**   possible resources that could be returned to this call. For example, if
**   there is a ResourceId associated with a particular compute blade upon which
**   the HPI User is running, and another associated with the chassis which
**   contains the compute blade, either could logically be returned as an
**   indication of a resource associated with the entity upon which the HPI User
**   was running. The function should return the ResourceId of the "smallest"
**   resource that is associated with the HPI User. So, in the example above,
**   the function should return the ResourceId of the compute blade.
**
**   Once the function has returned the ResourceId, the HPI User may issue 
**   further HPI calls using that ResourceId to learn the type of resource that
**   been identified.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceIdGet (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_OUT SaHpiResourceIdT     *ResourceId
);

/*******************************************************************************
**
** Name: saHpiEventLogInfoGet()
**
** Description:
**   This function retrieves the current number of entries in the Event Log,
**   total size of the Event Log, the time of the most recent update to the
**   Event Log, the current value of the Event Log's clock (i.e., timestamp that
**   would be placed on an entry at this moment), the enabled/disabled status of
**   the Event Log (see Section 6.4.8 on page 57), the overflow flag, and the
**   action taken by the Event Log if an overflow occurs.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen(). 
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   Info - [out] Pointer to the returned Event Log information.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the Info pointer is passed in as
**      NULL.
**
** Remarks:
**   The size field in the returned Event Log information indicates the maximum
**   number of entries that can be held in the Event Log.  This number should be
**   constant for a particular Event Log.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogInfoGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId,
    SAHPI_OUT SaHpiEventLogInfoT  *Info
);

/*******************************************************************************
**
** Name: saHpiEventLogEntryGet()
**
** Description:
**   This function retrieves an Event Log entry.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   EntryId - [in] Identifier of event log entry to retrieve. Reserved values:
**      * SAHPI_OLDEST_ENTRY           Oldest entry in the Event Log.
**      * SAHPI_NEWEST_ENTRY           Newest entry in the Event Log.
**      * SAHPI_NO_MORE_ENTRIES    Not valid for this parameter. Used only when
**         retrieving the next and previous EntryIds.
**   PrevEntryId - [out] Event Log entry identifier for the previous (older
**      adjacent) entry.  Reserved values:
**      * SAHPI_OLDEST_ENTRY       Not valid for this parameter. Used only for
**         the EntryId parameter.
**      * SAHPI_NEWEST_ENTRY       Not valid for this parameter. Used only for
**         the EntryId parameter.
**      * SAHPI_NO_MORE_ENTRIES    No more entries in the Event Log before the
**         one referenced by the EntryId parameter.
**   NextEntryId - [out] Event Log entry identifier for the next (newer
**      adjacent) entry. Reserved values:
**      * SAHPI_OLDEST_ENTRY     Not valid for this parameter. Used only for
**         the EntryId parameter.
**      * SAHPI_NEWEST_ENTRY     Not valid for this parameter. Used only for
**         the EntryId parameter.
**      * SAHPI_NO_MORE_ENTRIES  No more entries in the Event Log after the one
**         referenced by the EntryId parameter.
**   EventLogEntry - [out] Pointer to retrieved Event Log entry.
**   Rdr - [in/out] Pointer to structure to receive resource data record
**      associated with the Event Log entry, if available. If NULL, no RDR data
**      will be returned.
**   RptEntry - [in/out] Pointer to structure to receive RPT entry associated
**      with the Event Log entry, if available. If NULL, no RPT entry data will
**      be returned.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is 
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**   SA_ERR_HPI_NOT_PRESENT is returned when:
**      * The Event Log has no entries.
**      * The entry identified by EntryId is not present.
**   SA_ERR_HPI_INVALID_PARAMS is returned when:
**      * Any of PrevEntryId, NextEntryId and EventLogEntry pointers are passed
**         in as NULL.
**      * SAHPI_NO_MORE_ENTRIES is passed in to EntryId.
**
** Remarks:
**   The special EntryIds SAHPI_OLDEST_ENTRY and SAHPI_NEWEST_ENTRY are used to
**   select the oldest and newest entries, respectively, in the Event Log being
**   read. A returned NextEntryId of SAHPI_NO_MORE_ENTRIES indicates that the
**   newest entry has been returned; there are no more entries going forward
**   (time-wise) in the Event Log. A returned PrevEntryId of
**   SAHPI_NO_MORE_ENTRIES indicates that the oldest entry has been returned. 
**
**   To retrieve an entire list of entries going forward (oldest entry to newest
**   entry) in the Event Log, call this function first with an EntryId of
**   SAHPI_OLDEST_ENTRY and then use the returned NextEntryId as the EntryId in
**   the next call. Proceed until the NextEntryId returned is
**   SAHPI_NO_MORE_ENTRIES.
**
**   To retrieve an entire list of entries going backward (newest entry to
**   oldest entry) in the Event Log, call this function first with an EntryId of
**   SAHPI_NEWEST_ENTRY and then use the returned PrevEntryId as the EntryId in
**   the next call. Proceed until the PrevEntryId returned is 
**   SAHPI_NO_MORE_ENTRIES.
**
**   Event Logs may include RPT entries and resource data records associated
**   with the resource and sensor issuing an event along with the basic event
**   data in the Event Log. Because the system may be reconfigured after the
**   event was entered in the Event Log, this stored information may be
**   important to interpret the event. If the Event Log includes logged RPT
**   entries and/or RDRs, and if an HPI User provides a pointer to a structure
**   to receive this information, it will be returned along with the Event Log
**   entry.
**
**   If an HPI User provides a pointer for an RPT entry, but the Event Log does
**   not include a logged RPT entry for the Event Log entry being returned, 
**   RptEntry->ResourceCapabilities will be set to zero. No valid RptEntry will
**   have a zero Capabilities field value.
**
**   If an HPI User provides a pointer for an RDR, but the Event Log does not
**   include a logged RDR for the Event Log entry being returned, Rdr->RdrType
**   will be set to SAHPI_NO_RECORD.
**
**   The EntryIds returned via the PrevEntryId and NextEntryId parameters may
**   not be in sequential order, but will reflect the previous and next entries
**   in a chronological ordering of the Event Log, respectively.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogEntryGet (
    SAHPI_IN    SaHpiSessionIdT          SessionId,
    SAHPI_IN    SaHpiResourceIdT         ResourceId,
    SAHPI_IN    SaHpiEventLogEntryIdT    EntryId,
    SAHPI_OUT   SaHpiEventLogEntryIdT    *PrevEntryId,
    SAHPI_OUT   SaHpiEventLogEntryIdT    *NextEntryId,
    SAHPI_OUT   SaHpiEventLogEntryT      *EventLogEntry,
    SAHPI_INOUT SaHpiRdrT                *Rdr,
    SAHPI_INOUT SaHpiRptEntryT           *RptEntry
);

/*******************************************************************************
**
** Name: saHpiEventLogEntryAdd()
**
** Description:
**   This function enables an HPI user to add entries to the Event Log.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   EvtEntry - [in] Pointer to event data to write to the Event Log. The Event
**      field must be of type SAHPI_ET_USER, and the Source field must be 
**      SAHPI_UNSPECIFIED_RESOURCE_ID.
**
** Return Value:
**   SA_OK is returned if the event is successfully written in the Event Log;
**      otherwise, an error code is returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**   SA_ERR_HPI_INVALID_DATA is returned if the event DataLength is larger than
**      that supported by the implementation and reported in the Event Log info
**      record.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * EvtEntry pointer is passed in as NULL.
**      * Event structure passed via the EvtEntry parameter is not an event of
**         type SAHPI_ET_USER with the Source field set to 
**         SAHPI_UNSPECIFIED_RESOURCE_ID.
**      * The Severity is not one of the valid enumerated values for this type.
**      * SaHpiTextBufferT structure passed as part of the User Event structure
**         is not valid.  This would occur when:
**         * The DataType is not one of the enumerated values for that type, or
**         * The data field contains characters that are not legal according to
**            the value of DataType, or 
**         * The Language is not one of the enumerated values for that type when
**            the DataType is SAHPI_TL_TYPE_UNICODE or SAHPI_TL_TYPE_TEXT.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if the event cannot be written to the
**      Event Log because the Event Log is full, and the Event Log 
**      OverflowAction is SAHPI_EL_OVERFLOW_DROP.
**
** Remarks:
**   This function writes an event in the addressed Event Log. Nothing else is
**   done with the event.
**
**   If the Event Log is full, overflow processing occurs as defined by the
**   Event Log's OverflowAction setting, reported in the Event Log info record.
**   If, due to an overflow condition, the event is not written, or if existing
**   events are overwritten, then the OverflowFlag in the Event Log info record
**   will be set, just as it would be if an internally generated event caused
**   an overflow condition.  If the Event Log's OverflowAction is 
**   SAHPI_EL_OVERFLOW_DROP, then an error will be returned
**   (SA_ERR_HPI_OUT_OF_SPACE) indicating that the saHpiEventLogEntryAdd() 
**   function did not add the event to the Event Log.  If the Event Log's
**   OverflowAction is SAHPI_EL_OVERFLOW_OVERWRITE, then the 
**   saHpiEventLogEntryAdd() function will return SA_OK, indicating that the
**   event was added to the Event Log, even though an overflow occurred as a 
**   side-effect of this operation.  The overflow may be detected by checking 
**   the OverflowFlag in the Event Log info record.
**
**   Specific implementations of HPI may have restrictions on how much data may
**   be passed to the saHpiEventLogEntryAdd() function.  The Event Log info 
**   record reports the maximum DataLength that is supported by the Event Log
**   for User Events.  If saHpiEventLogEntryAdd() is called with a User Event
**   that has a larger DataLength than is supported, the event will not be added
**   to the Event Log, and an error will be returned. 
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogEntryAdd (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId,
    SAHPI_IN SaHpiEventT         *EvtEntry
);

/*******************************************************************************
**
** Name: saHpiEventLogClear()
**
** Description:
**   This function erases the contents of the specified Event Log.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code
**
** Remarks:
**   The OverflowFlag field in the Event Log info record will be reset when this
**   function is called.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogClear (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId
);

/*******************************************************************************
**
** Name: saHpiEventLogTimeGet()
**
** Description:
**   This function retrieves the current time from the Event Log's clock. This
**   clock is used to timestamp entries written into the Event Log.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**       Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   Time - [out] Pointer to the returned current Event Log time. 
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the Time pointer is passed in as
**      NULL.
**
** Remarks:
**   If the implementation cannot supply an absolute time value, then it may
**   supply a time relative to some system-defined epoch, such as system boot.
**   If the time value is less than or equal to SAHPI_TIME_MAX_RELATIVE, then it
**   is relative; if it is greater than SAHPI_TIME_MAX_RELATIVE, then it is
**   absolute. The HPI implementation must provide valid timestamps for Event
**   Log entries, using a default time base if no time has been set.  Thus, the
**   value SAHPI_TIME_UNSPECIFIED is never returned.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogTimeGet (
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_OUT SaHpiTimeT           *Time
);

/*******************************************************************************
**
** Name: saHpiEventLogTimeSet()
**
** Description:
**   This function sets the Event Log's clock, which is used to timestamp events
**   written into the Event Log.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   Time - [in] Time to which the Event Log clock should be set. 
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Time parameter is set to
**      SAHPI_TIME_UNSPECIFIED.
**   For situations when the underlying implementation cannot represent a time
**      value that is specified in Time, SA_ERR_HPI_INVALID_DATA is returned.
**
** Remarks:
**   If the Time parameter value is less than or equal to
**   SAHPI_TIME_MAX_RELATIVE, but not SAHPI_TIME_UNSPECIFIED, then it is
**   relative; if it is greater than SAHPI_TIME_MAX_RELATIVE, then it is
**   absolute.  Setting this parameter to the value SAHPI_TIME_UNSPECIFIED is
**   invalid and will result in an error return code of 
**   SA_ERR_HPI_INVALID_PARAMS.
**
**   Entries placed in the Event Log after this function is called will have
**   Event Log timestamps (i.e., the Timestamp field in the SaHpiEventLogEntryT
**   structure) based on the new time.  Setting the clock does not affect
**   existing Event Log entries.  If the time is set to a relative time,
**   subsequent entries placed in the Event Log will have an Event Log timestamp
**   expressed as a relative time; if the time is set to an absolute time,
**   subsequent entries will have an Event Log timestamp expressed as an
**   absolute time.  
**
**   This function only sets the Event Log time clock and does not have any
**   direct bearing on the timestamps placed on events (i.e., the Timestamp
**   field in the SaHpiEventT structure), or the timestamps placed in the domain
**   RPT info record.  Setting the clocks used to generate timestamps other than
**   Event Log timestamps is implementation-dependent, and should be documented
**   by the HPI implementation provider.
**
**   Some underlying implementations may not be able to handle the same relative
**   and absolute time ranges, as those defined in HPI.  Such limitations will
**   be documented.  When a time value is set in a region that is not supported
**   by the implementation, an error code of SA_ERR_HPI_INVALID_DATA will be
**   returned.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogTimeSet (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiResourceIdT     ResourceId,
    SAHPI_IN SaHpiTimeT           Time
);

/*******************************************************************************
**
** Name: saHpiEventLogStateGet()
**
** Description:
**   This function enables an HPI User to get the Event Log state. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen(). 
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   EnableState - [out] Pointer to the current Event Log enable state.  True
**      indicates that the Event Log is enabled; False indicates that it is
**      disabled.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the EnableState pointer is passed
**      in as NULL.
**
** Remarks:
**   If the Event Log is disabled, no events generated within the HPI
**   implementation will be added to the Event Log. Events may still be added to
**   the Event Log with the saHpiEventLogEntryAdd() function. When the Event Log
**   is enabled, events may be automatically added to the Event Log as they are
**   generated in a resource or a domain, however, it is implementation-specific
**   which events are automatically added to any Event Log.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogStateGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId,
    SAHPI_OUT SaHpiBoolT          *EnableState
);

/*******************************************************************************
**
** Name: saHpiEventLogStateSet()
**
** Description:
**   This function enables an HPI User to set the Event Log enabled state. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen(). 
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**      Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**   EnableState - [in] Event Log state to be set. True indicates that the Event
**      Log is to be enabled; False indicates that it is to be disabled.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**
** Remarks:
**   If the Event Log is disabled no events generated within the HPI
**   implementation will be added to the Event Log. Events may still be added to
**   the Event Log using the saHpiEventLogEntryAdd() function. When the Event 
**   Log is enabled events may be automatically added to the Event Log as they
**   are generated in a resource or a domain. The actual set of events that are
**   automatically added to any Event Log is implementation-specific.
**  
**   Typically, the HPI implementation will provide an appropriate default value
**   for this parameter, which may vary by resource. This function is provided
**   so that an HPI User can override the default, if desired.
**
**   If a resource hosting an Event Log is re-initialized (e.g., because of a
**   hot swap action), the HPI implementation may reset the value of this
**   parameter.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogStateSet (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiResourceIdT     ResourceId,
    SAHPI_IN SaHpiBoolT           EnableState
);

/*******************************************************************************
**
** Name: saHpiEventLogOverflowReset()
**
** Description:
**   This function resets the OverflowFlag in the Event Log info record of the
**   specified Event Log.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Identifier for the Resource containing the Event Log.
**       Set to SAHPI_UNSPECIFIED_RESOURCE_ID to address the Domain Event Log.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_CMD is returned if the implementation does not support
**      independent clearing of the OverflowFlag on this Event Log. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not have an Event
**      Log capability (SAHPI_CAPABILITY_EVENT_LOG) set.  Note this condition
**      only applies to Resource Event Logs.  Domain Event Logs are mandatory,
**      and should not return this code.
**
** Remarks:
**   The only effect of this function is to clear the OverflowFlag field in the
**   Event Log info record for the specified Event Log.  If the Event Log is 
**   still full, the OverflowFlag will be set again as soon as another entry
**   needs to be added to the Event Log.
**
**   Some Event Log implementations may not allow resetting of the OverflowFlag
**   except as a by-product of clearing the entire Event Log with the 
**   saHpiEventLogClear() function.  Such an implementation will return the
**   error code, SA_ERR_HPI_INVALID_CMD to this function.  The OverflowResetable
**   flag in the Event Log info record indicates whether or not the
**   implementation supports resetting the OverflowFlag without clearing the
**   Event Log.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventLogOverflowReset (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId
);

/*******************************************************************************
**
** Name: saHpiSubscribe()
**
** Description:
**   This function allows an HPI User to subscribe for events. This single call
**   provides subscription to all session events, regardless of event type or
**   event severity. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_DUPLICATE is returned when a subscription is already in place
**      for this session.
**
** Remarks:
**   Only one subscription is allowed per session, and additional subscribers
**   will receive an appropriate error code. No event filtering will be done by
**   the HPI implementation.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSubscribe (
    SAHPI_IN SaHpiSessionIdT      SessionId
);

/*******************************************************************************
**
** Name: saHpiUnsubscribe()
**
** Description:
**   This function removes the event subscription for the session. 
**
** Parameters:
**   SessionId - [in] Session for which event subscription will be closed.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_REQUEST is returned if the session is not currently
**      subscribed for events.
**
** Remarks:
**   After removal of a subscription, additional saHpiEventGet() calls will not
**   be allowed on the session unless an HPI User re-subscribes for events on
**   the session first. Any events that are still in the event queue when this
**   function is called will be cleared from it.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiUnsubscribe (
    SAHPI_IN SaHpiSessionIdT  SessionId
);

/*******************************************************************************
**
** Name: saHpiEventGet()
**
** Description:
**   This function allows an HPI User to get an event.  This call is only valid
**   within a session that has subscribed for events.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   Timeout - [in] The number of nanoseconds to wait for an event to arrive.
**      Reserved time out values:
**      * SAHPI_TIMEOUT_IMMEDIATE   Time out immediately if there are no events
**         available (non-blocking call).
**      * SAHPI_TIMEOUT_BLOCK       Call should not return until an event is
**         retrieved.
**   Event - [out] Pointer to the next available event.
**   Rdr - [in/out] Pointer to structure to receive the resource data associated
**      with the event.  If NULL, no RDR will be returned.
**   RptEntry - [in/out] Pointer to structure to receive the RPT entry
**      associated with the resource that generated the event.  If NULL, no RPT
**      entry will be returned.
**   EventQueueStatus - [in/out] Pointer to location to store event queue 
**      status.  If NULL, event queue status will not be returned.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_REQUEST is returned if an HPI User is not currently
**      subscribed for events in this session.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * Event pointer is passed in as NULL.
**      * Timeout parameter is not set to SAHPI_TIMEOUT_BLOCK, 
**         SAHPI_TIMEOUT_IMMEDIATE or a positive value.
**   SA_ERR_HPI_TIMEOUT is returned if no event is available to return within
**      the timeout period.  If SAHPI_TIMEOUT_IMMEDIATE is passed in the 
**      Timeout parameter, this error return will be used if there is no event
**      queued when the function is called.
**
** Remarks:
**   SaHpiEventGet() will also return an EventQueueStatus flag to an HPI User.
**   This flag indicates whether or not a queue overflow has occurred.  The 
**   overflow flag is set if any events were unable to be queued because of 
**   space limitations in the interface implementation.  The overflow flag is
**   reset whenever saHpiEventGet() is called.  
**
**   If there are one or more events on the event queue when this function is
**   called, it will immediately return the next event on the queue.  Otherwise,
**   if the Timeout parameter is SAHPI_TIMEOUT_IMMEDIATE, it will return
**   SA_ERR_HPI_TIMEOUT immediately.  Otherwise, it will block for time 
**   specified by the timeout parameter; if an event is added to the queue
**   within that time it will be returned immediately; if not, saHpiEventGet()
**   will return SA_ERR_HPI_TIMEOUT.  If the Timeout parameter is 
**   SAHPI_TIMEOUT_BLOCK, the saHpiEventGet() will block indefinitely, until
**   an event becomes available, and then return that event.  This provides for
**   notification of events as they occur.
**
**   If an HPI User provides a pointer for an RPT entry, but the event does not
**   include a valid ResourceId for a resource in the domain (e.g., OEM or USER
**   type event), then the RptEntry->ResourceCapabilities field will be set to
**   zero.  No valid RPT entry will have a zero ResourceCapabilities.
**
**   If an HPI User provides a pointer for an RDR, but there is no valid RDR
**   associated with the event being returned (e.g., returned event is not a
**   sensor event), then the Rdr->RdrType field will be set to SAHPI_NO_RECORD.
**
**   The timestamp reported in the returned event structure is the best
**   approximation an implementation has to when the event actually occurred.
**   The implementation may need to make an approximation (such as the time the
**   event was placed on the event queue) because it may not have access to the
**   actual time the event occurred.  The value SAHPI_TIME_UNSPECIFIED indicates
**   that the time of the event cannot be determined.
** 
**   If the implementation cannot supply an absolute timestamp, then it may
**   supply a timestamp relative to some system-defined epoch, such as system
**   boot.  If the timestamp value is less than or equal to 
**   SAHPI_TIME_MAX_RELATIVE, but not SAHPI_TIME_UNSPECIFIED, then it is
**   relative; if it is greater than SAHPI_TIME_MAX_RELATIVE, then it is
**   absolute.
**
**   If an HPI User passes a NULL pointer for the returned EventQueueStatus
**   pointer, the event status will not be returned, but the overflow flag, if
**   set, will still be reset.  Thus, if an HPI User needs to know about event
**   queue overflows, the EventQueueStatus parameter should never be NULL, and
**   the overflow flag should be checked after every call to saHpiEventGet().
**
**   If saHpiEventGet() is called with a timeout value other than 
**   SAHPI_TIMEOUT_IMMEDIATE, and the session is subsequently closed from
**   another thread, this function will return with SA_ERR_HPI_INVALID_SESSION.
**   If saHpiEventGet() is called with a timeout value other than 
**   SAHPI_TIMEOUT_IMMEDIATE, and an HPI User subsequently calls
**   saHpiUnsubscribe() from another thread, this function will return with
**   SA_ERR_HPI_INVALID_REQUEST.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventGet (
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiTimeoutT              Timeout,
    SAHPI_OUT SaHpiEventT               *Event,
    SAHPI_INOUT SaHpiRdrT               *Rdr,
    SAHPI_INOUT SaHpiRptEntryT          *RptEntry,
    SAHPI_INOUT SaHpiEvtQueueStatusT    *EventQueueStatus
);

/*******************************************************************************
**
** Name: saHpiEventAdd()
**
** Description:
**   This function enables an HPI User to add events to the HPI domain
**   identified by the SessionId.  The domain controller processes an event
**   added with this function as if the event originated from within the
**   domain.  The domain controller will attempt to publish events to all active
**   event subscribers and will attempt to log events in the Domain Event Log,
**   if room is available.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   EvtEntry - [in] Pointer to event to add to the domain.  Event must be of 
**      type SAHPI_ET_USER, and the Source field must be 
**      SAHPI_UNSPECIFIED_RESOURCE_ID.
**
** Return Value:
**   SA_OK is returned if the event is successfully added to the domain; 
**      otherwise, an error code is returned. 
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * EvtEntry parameter is NULL.
**      * Event structure passed via the EvtEntry parameter is not an event of
**         type SAHPI_ET_USER with the Source field being 
**         SAHPI_UNSPECIFIED_RESOURCE_ID.
**      * Event structure passed via the EvtEntry parameter has an invalid
**         Severity.
**      * SaHpiTextBufferT structure passed as part of the User Event structure
**         is not valid.  This would occur when:
**         * The DataType is not one of the enumerated values for that type, or 
**         * The data field contains characters that are not legal according to
**            the value of DataType, or 
**         * The Language is not one of the enumerated values for that type when
**            the DataType is SAHPI_TL_TYPE_UNICODE or SAHPI_TL_TYPE_TEXT. 
**   SA_ERR_HPI_INVALID_DATA is returned if the event data does not meet 
**      implementation-specific restrictions on how much event data may be
**      provided in a SAHPI_ET_USER event.
**   
** Remarks:
**   Specific implementations of HPI may have restrictions on how much data may
**   be included in a SAHPI_ET_USER event.  If more event data is provided than
**   can be processed, an error will be returned.  The event data size 
**   restriction for the SAHPI_ET_USER event type is provided in the 
**   UserEventMaxSize field in the domain Event Log info structure.  An HPI User
**   should call the function saHpiEventLogInfoGet() to retrieve the Event
**   Log info structure.
**  
**   The domain controller will attempt to publish the event to all sessions
**   within the domain with active event subscriptions; however, a session's 
**   event queue may overflow due to the addition of the new event.
**
**   The domain controller will attempt to log the event in the Domain Event
**   Log; however, the Domain Event Log may overflow due to the addition of the
**   new event
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiEventAdd (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiEventT          *EvtEntry
);

/*******************************************************************************
**
** Name: saHpiAlarmGetNext()
**
** Description:
**   This function allows retrieval of an alarm from the current set of alarms
**   held in the Domain Alarm Table (DAT).  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   Severity - [in] Severity level of alarms to retrieve.  Set to 
**      SAHPI_ALL_SEVERITIES to retrieve alarms of any severity; otherwise, set
**      to requested severity level.
**   UnacknowledgedOnly - [in] Set to True to indicate only unacknowledged
**      alarms should be returned.  Set to False to indicate either an
**      acknowledged or unacknowledged alarm may be returned.
**   Alarm - [in/out] Pointer to the structure to hold the returned alarm entry.
**      Also, on input, Alarm->AlarmId and Alarm->Timestamp are used to
**      identify the previous alarm.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_INVALID_PARAMS is returned when:
**      * Severity is not one of the valid enumerated values for this type.
**      * The Alarm parameter is passed in as NULL.
**   SA_ERR_HPI_NOT_PRESENT is returned:
**      * If there are no additional alarms in the DAT that meet the criteria
**         specified by the Severity and UnacknowledgedOnly parameters.
**      * If the passed Alarm->AlarmId field was set to SAHPI_FIRST_ENTRY and
**         there are no alarms in the DAT that meet the criteria specified by
**         the Severity and UnacknowledgedOnly parameters.
**   SA_ERR_HPI_INVALID_DATA is returned if the passed Alarm->AlarmId matches an
**      alarm in the DAT, but the passed Alarm->Timestamp does not match the
**      timestamp of that alarm.
**   
** Remarks:
**   All alarms contained in the DAT are maintained in the order in which they
**   were added.  This function will return the next alarm meeting the
**   specifications given by an HPI User that was added to the DAT after the
**   alarm whose AlarmId and Timestamp is passed by an HPI User, even if the
**   alarm associated with the AlarmId and Timestamp has been deleted.  If
**   SAHPI_FIRST_ENTRY is passed as the AlarmId, the first alarm in the DAT
**   meeting the specifications given by an HPI User is returned.  
**
**   Alarm selection can be restricted to only alarms of a specified severity,
**   and/or only unacknowledged alarms.
**
**   To retrieve all alarms contained within the DAT meeting specific
**   requirements, call saHpiAlarmGetNext() with the Alarm->AlarmId field set to
**   SAHPI_FIRST_ENTRY and the Severity and UnacknowledgedOnly parameters set to
**   select what alarms should be returned.  Then, repeatedly call
**   saHpiAlarmGetNext() passing the previously returned alarm as the Alarm
**   parameter, and the same values for Severity and UnacknowledgedOnly until
**   the function returns with the error code SA_ERR_HPI_NOT_PRESENT.
**
**   SAHPI_FIRST_ENTRY and SAHPI_LAST_ENTRY are reserved AlarmId values, and 
**   will never be assigned to an alarm in the DAT.
**
**   The elements AlarmId and Timestamp are used in the Alarm parameter to
**   identify the previous alarm; the next alarm added to the table after this
**   alarm that meets the Severity and UnacknowledgedOnly requirements will be
**   returned.  Alarm->AlarmId may be set to SAHPI_FIRST_ENTRY to select the
**   first alarm in the DAT meeting the Severity and UnacknowledgedOnly
**   requirements.  If Alarm->AlarmId is SAHPI_FIRST_ENTRY, then 
**   Alarm->Timestamp is ignored.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAlarmGetNext( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiSeverityT             Severity,
    SAHPI_IN SaHpiBoolT                 UnacknowledgedOnly,
    SAHPI_INOUT SaHpiAlarmT             *Alarm
);

/*******************************************************************************
**
** Name: saHpiAlarmGet()
**
** Description:
**   This function allows retrieval of a specific alarm in the Domain Alarm
**   Table (DAT) by referencing its AlarmId.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   AlarmId - [in] AlarmId of the alarm to be retrieved from the DAT.
**   Alarm - [out] Pointer to the structure to hold the returned alarm entry. 
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_NOT_PRESENT is returned if the requested AlarmId does not
**      correspond to an alarm contained in the DAT.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Alarm parameter is passed in
**      as NULL.
**   
** Remarks:
**   SAHPI_FIRST_ENTRY and SAHPI_LAST_ENTRY are reserved AlarmId values, and
**   will never be assigned to an alarm in the DAT.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAlarmGet( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiAlarmIdT              AlarmId,
    SAHPI_OUT SaHpiAlarmT               *Alarm
);

/*******************************************************************************
**
** Name: saHpiAlarmAcknowledge()
**
** Description:
**   This function allows an HPI User to acknowledge a single alarm entry or a
**   group of alarm entries by severity.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   AlarmId - [in] Identifier of the alarm to be acknowledged.  Reserved
**      AlarmId values:
**      * SAHPI_ENTRY_UNSPECIFIED     Ignore this parameter.
**   Severity - [in] Severity level of alarms to acknowledge.  Ignored unless
**      AlarmId is SAHPI_ENTRY_UNSPECIFIED.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_NOT_PRESENT is returned if an alarm entry identified by the 
**      AlarmId parameter does not exist in the DAT.
**   SA_ERR_HPI_INVALID_PARAMS is returned if AlarmId is SAHPI_ENTRY_UNSPECIFIED
**      and Severity is not one of the valid enumerated values for this type.
**   
** Remarks:
**   An HPI User acknowledges an alarm to indicate that it is aware of the alarm
**   and to influence platform-specific alarm annunciation that may be provided
**   by the implementation.  Typically, an implementation ignores acknowledged
**   alarms when announcing an alarm on annunciation devices such as audible
**   sirens and dry contact closures.  However, alarm annunciation is 
**   implementation-specific.
**
**   An acknowledged alarm will have the Acknowledged field in the alarm entry
**   set to True.
**
**   Alarms are acknowledged by one of two ways: a single alarm entry by AlarmId
**   regardless of severity or as a group of alarm entries by Severity 
**   regardless of AlarmId.  
**
**   To acknowledge all alarms contained within the DAT, set the Severity
**   parameter to SAHPI_ALL_SEVERITIES, and set the AlarmId parameter to 
**   SAHPI_ENTRY_UNSPECIFIED.
**
**   To acknowledge all alarms of a specific severity contained within the DAT,
**   set the Severity parameter to the appropriate value, and set the AlarmId
**   parameter to SAHPI_ENTRY_UNSPECIFIED.
**
**   To acknowledge a single alarm entry, set the AlarmId parameter to a value
**   other than SAHPI_ENTRY_UNSPECIFIED.  The AlarmId must be a valid identifier
**   for an alarm entry present in the DAT at the time of the function call.
**
**   If an alarm has been previously acknowledged, acknowledging it again has no
**   effect.  However, this is not an error.
**
**   If the AlarmId parameter has a value other than SAHPI_ENTRY_UNSPECIFIED,
**   the Severity parameter is ignored.
** 
**   If the AlarmId parameter is passed as SAHPI_ENTRY_UNSPECIFIED, and no
**   alarms are present that meet the requested Severity, this function will
**   have no effect.  However, this is not an error.
**
**   SAHPI_ENTRY_UNSPECIFIED is defined as the same value as SAHPI_FIRST_ENTRY,
**   so using either symbol will have the same effect.  However, 
**   SAHPI_ENTRY_UNSPECIFIED should be used with this function for clarity.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAlarmAcknowledge( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiAlarmIdT              AlarmId,
    SAHPI_IN SaHpiSeverityT             Severity
);

/*******************************************************************************
**
** Name: saHpiAlarmAdd()
**
** Description:
**   This function is used to add a User Alarm to the DAT.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   Alarm - [in/out] Pointer to the alarm entry structure that contains the new
**      User Alarm to add to the DAT.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Alarm pointer is passed in
**      as NULL.
**   SA_ERR_HPI_INVALID_PARAMS is returned when Alarm->Severity is not one of
**      the following enumerated values: SAHPI_MINOR, SAHPI_MAJOR, or 
**      SAHPI_CRITICAL.
**   SA_ERR_HPI_INVALID_PARAMS is returned when Alarm->AlarmCond.Type is not
**      SAHPI_STATUS_COND_TYPE_USER.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if the DAT is not able to add an
**      additional User Alarm due to space limits or limits imposed on the
**      number of User Alarms permitted in the DAT.
**   
** Remarks:
**   The AlarmId, and Timestamp fields within the Alarm parameter are not used
**   by this function.  Instead, on successful completion, these fields are set
**   to new values associated with the added alarm.  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAlarmAdd( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_INOUT SaHpiAlarmT             *Alarm
);

/*******************************************************************************
**
** Name: saHpiAlarmDelete()
**
** Description:
**   This function allows an HPI User to delete a single User Alarm or a group
**   of User Alarms from the DAT.  Alarms may be deleted individually by
**   specifying a specific AlarmId, or they may be deleted as a group by
**   specifying a Severity.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   AlarmId - [in] Alarm identifier of the alarm entry to delete.  Reserved
**      values:
**      * SAHPI_ENTRY_UNSPECIFIED     Ignore this parameter.
**   Severity - [in] Severity level of alarms to delete.  Ignored unless AlarmId
**      is SAHPI_ENTRY_UNSPECIFIED.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_INVALID_PARAMS is returned if AlarmId is SAHPI_ENTRY_UNSPECIFIED
**      and Severity is not one of the valid enumerated values for this type.
**   SA_ERR_HPI_NOT_PRESENT is returned if an alarm entry identified by the
**      AlarmId parameter does not exist in the DAT.
**   SA_ERR_HPI_READ_ONLY is returned if the AlarmId parameter indicates a
**      non-User Alarm.
**   
** Remarks:
**   Only User Alarms added to the DAT can be deleted.  When deleting alarms by
**   severity, only User Alarms of the requested severity will be deleted.  
**
**   To delete a single, specific alarm, set the AlarmId parameter to a value
**   representing an actual User Alarm in the DAT.  The Severity parameter is
**   ignored when the AlarmId parameter is set to a value other than
**   SAHPI_ENTRY_UNSPECIFIED.
**
**   To delete a group of User Alarms, set the AlarmId parameter to 
**   SAHPI_ENTRY_UNSPECIFIED, and set the Severity parameter to identify which 
**   severity of alarms should be deleted.  To clear all User Alarms contained
**   within the DAT, set the Severity parameter to SAHPI_ALL_SEVERITIES. 
**
**   If the AlarmId parameter is passed as SAHPI_ENTRY_UNSPECIFIED, and no User
**   Alarms are present that meet the specified Severity, this function will
**   have no effect.  However, this is not an error.
**
**   SAHPI_ENTRY_UNSPECIFIED is defined as the same value as SAHPI_FIRST_ENTRY,
**   so using either symbol will have the same effect.  However,
**   SAHPI_ENTRY_UNSPECIFIED should be used with this function for clarity.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAlarmDelete( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiAlarmIdT              AlarmId,
    SAHPI_IN SaHpiSeverityT             Severity
);

/*******************************************************************************
**
** Name: saHpiRdrGet()
**
** Description:
**   This function returns a resource data record from the addressed resource.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   EntryId - [in] Identifier of the RDR entry to retrieve. Reserved EntryId
**      values:
**      * SAHPI_FIRST_ENTRY     Get first entry.
**      * SAHPI_LAST_ENTRY      Reserved as delimiter for end of list. Not a
**         valid entry identifier.
**   NextEntryId - [out] Pointer to location to store EntryId of next entry in
**      RDR repository.
**   Rdr - [out] Pointer to the structure to receive the requested resource data
**      record.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource contains no RDR records
**      (and thus does not have the SAHPI_CAPABILITY_RDR flag set in its RPT
**      entry).
**   SA_ERR_HPI_NOT_PRESENT is returned if an EntryId (other than
**      SAHPI_FIRST_ENTRY) is passed that does not correspond to an actual
**      EntryId in the resource's RDR repository.
**   SA_ERR_HPI_INVALID_PARAMS is returned if:
**      * SAHPI_LAST_ENTRY is passed in to EntryId.
**      * NextEntryId pointer is passed in as NULL.
**      * Rdr pointer is passed in as NULL.
**   
** Remarks:
**   Submitting an EntryId of SAHPI_FIRST_ENTRY results in the first RDR being
**   read. A returned NextEntryId of SAHPI_LAST_ENTRY indicates the last RDR has
**   been returned. A successful retrieval will include the next valid EntryId.
**   To retrieve the entire list of RDRs, call this function first with an
**   EntryId of SAHPI_FIRST_ENTRY and then use the returned NextEntryId in the
**   next call. Proceed until the NextEntryId returned is SAHPI_LAST_ENTRY.
**
**   A resource's RDR repository is static over the lifetime of the resource;
**   therefore no precautions are required against changes to the content of the
**   RDR repository while it is being accessed.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiRdrGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_IN  SaHpiEntryIdT         EntryId,
    SAHPI_OUT SaHpiEntryIdT         *NextEntryId,
    SAHPI_OUT SaHpiRdrT             *Rdr
);

/*******************************************************************************
**
** Name: saHpiRdrGetByInstrumentId()
**
** Description:
**   This function returns the Resource Data Record (RDR) for a specific
**   management instrument hosted by the addressed resource.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   RdrType - [in] Type of RDR being requested.
**   InstrumentId - [in] Instrument number identifying the specific RDR to be
**      returned.  This is a sensor number, control number, watchdog timer
**      number, IDR number, or annunciator number, depending on the value of
**      the RdrType parameter.
**   Rdr - [out] Pointer to the structure to receive the requested RDR.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the:
**      * Resource contains no RDR records (and thus does not have the 
**         SAHPI_CAPABILITY_RDR flag set in its RPT entry).
**      * Type of management instrument specified in the RdrType parameter is
**         not supported by the resource, as indicated by the Capability field
**         in its RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the specific management instrument
**      identified in the InstrumentId parameter is not present in the addressed
**      resource.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the:
**      * RdrType parameter is not a valid enumerated value for the type.
**      * RdrType is SAHPI_NO_RECORD.
**      * Rdr pointer is passed in as NULL.
**   
** Remarks:
**   The RDR to be returned is identified by RdrType (sensor, control, watchdog
**   timer, inventory data repository, or annunciator) and InstrumentId (sensor
**   number, control number, watchdog number, IDR number, or annunciator number)
**   for the specific management instrument for the RDR being requested.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiRdrGetByInstrumentId (
    SAHPI_IN  SaHpiSessionIdT        SessionId,
    SAHPI_IN  SaHpiResourceIdT       ResourceId,
    SAHPI_IN  SaHpiRdrTypeT          RdrType,
    SAHPI_IN  SaHpiInstrumentIdT     InstrumentId,
    SAHPI_OUT SaHpiRdrT              *Rdr
);

/*******************************************************************************
**
** Name: saHpiSensorReadingGet()
**
** Description:
**   This function is used to retrieve a sensor reading. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the sensor reading is being
**      retrieved.
**   Reading - [in/out] Pointer to a structure to receive sensor reading values
**      If NULL, the sensor reading value will not be returned.
**   EventState - [in/out] Pointer to location to receive sensor event states.
**      If NULL, the sensor event states will not be returned.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the sensor is currently disabled.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   
** Remarks:
**   For sensors that return a type of SAHPI_SENSOR_READING_TYPE_BUFFER, the
**   format of the returned data buffer is implementation-specific.
**
**   If the sensor does not provide a reading, the Reading structure returned by
**   the saHpiSensorReadingGet() function will indicate the reading is not
**   supported by setting the IsSupported flag to False.
**
**   If the sensor does not support any event states, a value of 0x0000 will be
**   returned for the EventState value.  This is indistinguishable from the
**   return for a sensor that does support event states, but currently has no
**   event states asserted.  The Sensor RDR Events field can be examined to
**   determine if the sensor supports any event states.
**
**   It is legal for both the Reading parameter and the EventState parameter to
**   be NULL.  In this case, no data is returned other than the return code.
**   This can be used to determine if a sensor is present and enabled without
**   actually returning current sensor data.  If the sensor is present and
**   enabled, SA_OK is returned; otherwise, an error code is returned.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorReadingGet (
    SAHPI_IN    SaHpiSessionIdT       SessionId,
    SAHPI_IN    SaHpiResourceIdT      ResourceId,
    SAHPI_IN    SaHpiSensorNumT       SensorNum,
    SAHPI_INOUT SaHpiSensorReadingT   *Reading,
    SAHPI_INOUT SaHpiEventStateT      *EventState
);

/*******************************************************************************
**
** Name: saHpiSensorThresholdsGet()
**
** Description:
**   This function retrieves the thresholds for the given sensor. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which threshold values are being
**      retrieved.
**   SensorThresholds - [out] Pointer to returned sensor thresholds.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry. 
**   SA_ERR_HPI_INVALID_PARAMS is returned if the SensorThresholds pointer is
**      passed in as NULL.
**   SA_ERR_HPI_INVALID_CMD is returned if:
**      * Getting a threshold on a sensor that is not a threshold type.
**      * The sensor does not have any readable threshold values.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   
** Remarks:
**   This function only applies to sensors that support readable thresholds, as
**   indicated by the IsAccessible field in the SaHpiSensorThdDefnT structure
**   of the sensor's RDR being set to True and the ReadThold field in the same
**   structure having a non-zero value.
**
**   For thresholds that do not apply to the identified sensor, the IsSupported
**   flag of the threshold value field will be set to False.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorThresholdsGet (
    SAHPI_IN  SaHpiSessionIdT            SessionId,
    SAHPI_IN  SaHpiResourceIdT           ResourceId,
    SAHPI_IN  SaHpiSensorNumT            SensorNum,
    SAHPI_OUT SaHpiSensorThresholdsT     *SensorThresholds
);


/*******************************************************************************
**
** Name: saHpiSensorThresholdsSet()
**
** Description:
**   This function sets the specified thresholds for the given sensor. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which threshold values are being set.
**   SensorThresholds - [in] Pointer to the sensor thresholds values being set.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_DATA is returned if any of the threshold values are
**      provided in a format not supported by the sensor.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   SA_ERR_HPI_INVALID_CMD is returned when:
**      * Writing to a threshold that is not writable.
**      * Setting a threshold on a sensor that is not a threshold type as
**         indicated by the IsAccessible field of the SaHpiSensorThdDefnT 
**         structure.
**      * Setting a threshold outside of the Min-Max range as defined by the 
**         Range field of the SensorDataFormat of the RDR.
**   SA_ERR_HPI_INVALID_DATA is returned when:
**      * Thresholds are set out-of-order (see Remarks).
**      * A negative hysteresis value is provided.
**   
** Remarks:
**   This function only applies to sensors that support writable thresholds,
**   as indicated by the IsAccessible field in the SaHpiSensorThdDefnT
**   structure of the sensor's RDR being set to True and the WriteThold field
**   in the same structure having a non-zero value.
** 
**   The type of value provided for each threshold setting must correspond to
**   the reading format supported by the sensor, as defined by the reading type
**   in the DataFormat field of the sensor's RDR (saHpiSensorRecT).
**  
**   Sensor thresholds cannot be set outside of the range defined by the Range
**   field of the SensorDataFormat of the Sensor RDR.  If SAHPI_SRF_MAX
**   indicates that a maximum reading exists, no sensor threshold may be set
**   greater than the Max value.  If SAHPI_SRF_MIN indicates that a minimum
**   reading exists, no sensor threshold may be set less than the Min value. 
** 
**   Thresholds are required to be set progressively in-order, so that 
**   Upper Critical >= Upper Major >= Upper Minor >= Lower Minor >= Lower Major
**   >= Lower Critical.  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorThresholdsSet (
    SAHPI_IN  SaHpiSessionIdT          SessionId,
    SAHPI_IN  SaHpiResourceIdT         ResourceId,
    SAHPI_IN  SaHpiSensorNumT          SensorNum,
    SAHPI_IN  SaHpiSensorThresholdsT   *SensorThresholds
);

/*******************************************************************************
**
** Name: saHpiSensorTypeGet()
**
** Description:
**   This function retrieves the sensor type and event category for the
**   specified sensor.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the type is being retrieved.
**   Type - [out] Pointer to returned enumerated sensor type for the specified
**      sensor.
**   Category - [out] Pointer to location to receive the returned sensor event
**      category.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**     as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * Type pointer is passed in as NULL.
**      * Category pointer is passed in as NULL.
**   
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorTypeGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_IN  SaHpiSensorNumT       SensorNum,
    SAHPI_OUT SaHpiSensorTypeT      *Type,
    SAHPI_OUT SaHpiEventCategoryT   *Category
);

/*******************************************************************************
**
** Name: saHpiSensorEnableGet()
**
** Description:
**   This function returns the current sensor enable status for an addressed
**   sensor.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the sensor enable status is being
**      requested.
**   SensorEnabled - [out] Pointer to the location to store the sensor enable
**      status.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the SensorEnabled pointer is set
**      to NULL.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   
** Remarks:
**   The SaHpiBoolT value pointed to by the SensorEnabled parameter will be set
**   to True if the sensor is enabled, or False if the sensor is disabled.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorEnableGet (
    SAHPI_IN  SaHpiSessionIdT         SessionId,
    SAHPI_IN  SaHpiResourceIdT        ResourceId,
    SAHPI_IN  SaHpiSensorNumT         SensorNum,
    SAHPI_OUT SaHpiBoolT              *SensorEnabled
);

/*******************************************************************************
**
** Name: saHpiSensorEnableSet()
**
** Description:
**   This function sets the sensor enable status for an addressed sensor.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the sensor enable status is being
**      set.
**   SensorEnabled - [in] Sensor enable status to be set for the sensor.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   SA_ERR_HPI_READ_ONLY is returned if the sensor does not support changing
**      the enable status (i.e., the EnableCtrl field in the Sensor RDR is set
**      to False).
**   
** Remarks:
**   If a sensor is disabled, any calls to saHpiSensorReadingGet() for that
**   sensor will return an error, and no events will be generated for the
**   sensor.
**
**   Calling saHpiSensorEnableSet() with a SensorEnabled parameter of True 
**   will enable the sensor.  A SensorEnabled parameter of False will disable
**   the sensor.
**
**   If the sensor enable status changes as the result of this function call,
**   an event will be generated.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorEnableSet (
    SAHPI_IN  SaHpiSessionIdT         SessionId,
    SAHPI_IN  SaHpiResourceIdT        ResourceId,
    SAHPI_IN  SaHpiSensorNumT         SensorNum,
    SAHPI_IN  SaHpiBoolT              SensorEnabled
);

/*******************************************************************************
**
** Name: saHpiSensorEventEnableGet()
**
** Description:
**   This function returns the current sensor event enable status for an
**   addressed sensor.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the sensor event enable status is
**      being requested.
**   SensorEventsEnabled - [out] Pointer to the location to store the sensor
**      event enable status.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the SensorEventsEnabled pointer
**      is set to NULL.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   
** Remarks:
**   The SaHpiBoolT value pointed to by the SensorEventsEnabled parameter will
**   be set to True if event generation for the sensor is enabled, or False if
**   event generation for the sensor is disabled.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorEventEnableGet (
    SAHPI_IN  SaHpiSessionIdT         SessionId,
    SAHPI_IN  SaHpiResourceIdT        ResourceId,
    SAHPI_IN  SaHpiSensorNumT         SensorNum,
    SAHPI_OUT SaHpiBoolT              *SensorEventsEnabled
);

/*******************************************************************************
**
** Name: saHpiSensorEventEnableSet()
**
** Description:
**   This function sets the sensor event enable status for an addressed sensor.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the sensor enable status is being
**      set.
**   SensorEventsEnabled - [in] Sensor event enable status to be set for the
**      sensor.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support sensors,
**      as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   SA_ERR_HPI_READ_ONLY is returned if the sensor does not support changing
**      the event enable status (i.e., the EventCtrl field in the Sensor RDR is
**      set to SAHPI_SEC_READ_ONLY).
**   
** Remarks:
**   If event generation for a sensor is disabled, no events will be generated
**   as a result of the assertion or deassertion of any event state, regardless
**   of the setting of the assert or deassert event masks for the sensor.  If
**   event generation for a sensor is enabled, events will be generated when
**   event states are asserted or deasserted, according to the settings of the
**   assert and deassert event masks for the sensor.  Event states may still be
**   read for a sensor even if event generation is disabled, by using the 
**   saHpiSensorReadingGet() function.
**
**   Calling saHpiSensorEventEnableSet() with a SensorEventsEnabled parameter
**   of True will enable event generation for the sensor.  A SensorEventsEnabled
**   parameter of False will disable event generation for the sensor.
**
**   If the sensor event enabled status changes as a result of this function
**   call, an event will be generated.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorEventEnableSet (
    SAHPI_IN  SaHpiSessionIdT         SessionId,
    SAHPI_IN  SaHpiResourceIdT        ResourceId,
    SAHPI_IN  SaHpiSensorNumT         SensorNum,
    SAHPI_IN  SaHpiBoolT              SensorEventsEnabled
);

/*******************************************************************************
**
** Name: saHpiSensorEventMasksGet()
**
** Description:
**   This function returns the assert and deassert event masks for a sensor.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the event enable configuration is
**      being requested.
**   AssertEventMask - [in/out] Pointer to location to store sensor assert event
**      mask.  If NULL, assert event mask is not returned.
**   DeassertEventMask - [in/out] Pointer to location to store sensor deassert
**      event mask.  If NULL, deassert event mask is not returned.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      sensors, as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT
**      entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   
** Remarks:
**   Two bit-mask values are returned by the saHpiSensorEventMasksGet()
**   function; one for the sensor assert event mask, and one for the sensor
**   deassert event mask. A bit set to '1' in the AssertEventMask value
**   indicates that an event will be generated by the sensor when the
**   corresponding event state for that sensor changes from deasserted to
**   asserted.  A bit set to '1' in the DeassertEventMask value indicates that
**   an event will be generated by the sensor when the corresponding event
**   state for that sensor changes from asserted to deasserted. 
**
**   Events will only be generated by the sensor if the appropriate
**   AssertEventMask or DeassertEventMask bit is set, sensor events are
**   enabled, and the sensor is enabled.
**
**   For sensors hosted by resources that have the
**   SAHPI_CAPABILITY_EVT_DEASSERTS flag set in its RPT entry, the
**   AssertEventMask and the DeassertEventMask values will always be the same.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorEventMasksGet (
    SAHPI_IN  SaHpiSessionIdT         SessionId,
    SAHPI_IN  SaHpiResourceIdT        ResourceId,
    SAHPI_IN  SaHpiSensorNumT         SensorNum,
    SAHPI_INOUT SaHpiEventStateT      *AssertEventMask,
    SAHPI_INOUT SaHpiEventStateT      *DeassertEventMask
);

/*******************************************************************************
**
** Name: saHpiSensorEventMasksSet()
**
** Description:
**   This function provides the ability to change the settings of the sensor
**   assert and deassert event masks.  Two parameters contain bit-mask values
**   indicating which bits in the sensor assert and deassert event masks should
**   be updated.  In addition, there is an Action parameter.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   SensorNum - [in] Sensor number for which the event enable configuration
**      is being set.
**   Action - [in] Enumerated value describing what change should be made to
**      the sensor event masks:
**      * SAHPI_SENS_ADD_EVENTS_TO_MASKS - for each bit set in the
**         AssertEventMask and DeassertEventMask parameters, set the
**         corresponding bit in the sensor's assert and deassert event masks,
**         respectively.
**      * SAHPI_SENS_REMOVE_EVENTS_FROM_MASKS - for each bit set in the
**         AssertEventMask and DeassertEventMask parameters, clear the
**         corresponding bit in the sensor's assert and deassert event masks,
**         respectively.
**   AssertEventMask - [in] Bit mask or special value indicating which bits in
**      the sensor's assert event mask should be set or cleared. (But see
**      Remarks concerning resources with the SAHPI_EVT_DEASSERTS_CAPABILITY
**      flag set.)
**   DeassertEventMask - [in] Bit mask or special value indicating which bits
**      in the sensor's deassert event mask should be set or cleared.  (But see
**      Remarks concerning resources with the SAHPI_EVT_DEASSERTS_CAPABILITY
**      flag set.)
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      sensors, as indicated by SAHPI_CAPABILITY_SENSOR in the resource's RPT
**      entry.
**   SA_ERR_HPI_INVALID_DATA is returned if the Action parameter is 
**      SAHPI_SENS_ADD_EVENTS_TO_MASKS, and either of the AssertEventMask or 
**      DeassertEventMask parameters include a bit for an event state that is
**      not supported by the sensor.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the Action parameter is out of 
**      range.
**   SA_ERR_HPI_NOT_PRESENT is returned if the sensor is not present.
**   SA_ERR_HPI_READ_ONLY is returned if the sensor does not support updating
**      the assert and deassert event masks (i.e., the EventCtrl field in the
**      Sensor RDR is set to SAHPI_SEC_READ_ONLY_MASKS or SAHPI_SEC_READ_ONLY).
**   
** Remarks:
**   The bits in the sensor assert and deassert event masks that correspond to
**   '1' bits in the bit-mask parameters will be set or cleared, as indicated
**   by the Action parameter.  The bits in the sensor assert and deassert event
**   masks corresponding to '0' bits in the bit-mask parameters will be
**   unchanged.
**
**   Assuming that a sensor is enabled and event generation for the sensor is
**   enabled, then for each bit set in the sensor's assert event mask, an event
**   will be generated when the sensor's corresponding event state changes from
**   deasserted to asserted.  Similarly, for each bit set in the sensor's
**   deassert event mask, an event will be generated when the sensor's
**   corresponding event state changes from asserted to deasserted.
**
**   For sensors hosted by a resource that has the
**   SAHPI_CAPABILITY_EVT_DEASSERTS flag set in its RPT entry, the assert and
**   deassert event masks cannot be independently configured.  When 
**   saHpiSensorEventMasksSet() is called for sensors in a resource with this
**   capability, the DeassertEventMask parameter is ignored, and the
**   AssertEventMask parameter is used to determine which bits to set or clear
**   in both the assert event mask and deassert event mask for the sensor.
** 
**   The AssertEventMask or DeassertEventMask parameter may be set to the
**   special value, SAHPI_ALL_EVENT_STATES, indicating that all event states
**   supported by the sensor should be added to or removed from, the
**   corresponding sensor event mask.
**
**   If the sensor assert and/or deassert event masks change as a result of
**   this function call, an event will be generated.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiSensorEventMasksSet (
    SAHPI_IN  SaHpiSessionIdT                 SessionId,
    SAHPI_IN  SaHpiResourceIdT                ResourceId,
    SAHPI_IN  SaHpiSensorNumT                 SensorNum,
    SAHPI_IN  SaHpiSensorEventMaskActionT     Action,
    SAHPI_IN  SaHpiEventStateT                AssertEventMask,
    SAHPI_IN  SaHpiEventStateT                DeassertEventMask
);

/*******************************************************************************
**
** Name: saHpiControlTypeGet()
**
** Description:
**   This function retrieves the control type of a control object.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   CtrlNum - [in] Control number for which the type is being retrieved.
**   Type - [out] Pointer to SaHpiCtrlTypeT variable to receive the enumerated
**      control type for the specified control.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      controls, as indicated by SAHPI_CAPABILITY_CONTROL in the resource's
**      RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the control is not present.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the Type pointer is passed in as
**      NULL.
**   
** Remarks:
**   The Type parameter must point to a variable of type SaHpiCtrlTypeT. Upon
**   successful completion, the enumerated control type is returned in the
**   variable pointed to by Type.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiControlTypeGet (
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_IN  SaHpiResourceIdT    ResourceId,
    SAHPI_IN  SaHpiCtrlNumT       CtrlNum,
    SAHPI_OUT SaHpiCtrlTypeT      *Type
);

/*******************************************************************************
**
** Name: saHpiControlGet()
**
** Description:
**   This function retrieves the current state and mode of a control object.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   CtrlNum - [in] Control number for which the state and mode are being
**      retrieved.
**   CtrlMode - [out] Pointer to the mode of the control.  If NULL, the 
**      control's mode will not be returned.
**   CtrlState - [in/out] Pointer to a control data structure into which the
**      current control state will be placed. For text controls, the line 
**      number to read is passed in via CtrlState->StateUnion.Text.Line. 
**      If NULL, the control's state will not be returned.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_CMD is returned if the control is a write-only control,
**      as indicated by the WriteOnly flag in the control's RDR (see remarks).
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      controls, as indicated by the SAHPI_CAPABILITY_CONTROL in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_DATA is returned if the addressed control is a text
**      control, and the line number passed in CtrlState->StateUnion.Text.Line
**      does not exist in the control and is not SAHPI_TLN_ALL_LINES. 
**   SA_ERR_HPI_NOT_PRESENT is returned if the control is not present.
**   
** Remarks:
**   Note that the CtrlState parameter is both an input and an output parameter
**   for this function.  This is necessary to support line number inputs for
**   text controls, as discussed below.  
**
**   In some cases, the state of a control may be set, but the corresponding
**   state cannot be read at a later time.  Such controls are delineated with
**   the WriteOnly flag in the Control's RDR.
**
**   Note that text controls are unique in that they have a state associated
**   with each line of the control - the state being the text on that line. The
**   line number to be read is passed in to saHpiControlGet() via 
**   CtrlState->StateUnion.Text.Line; the contents of that line of the control
**   will be returned in CtrlState->StateUnion.Text.Text.  The first line of 
**   the text control is line number "1".
**
**   If the line number passed in is SAHPI_TLN_ALL_LINES, then
**   saHpiControlGet() will return the entire text of the control, or as much
**   of it as will fit in a single SaHpiTextBufferT, in
**   CtrlState->StateUnion.Text.Text. This value will consist of the text of
**   all the lines concatenated, using the maximum number of characters for
**   each line (no trimming of trailing blanks).
**
**   Note that depending on the data type and language, the text may be encoded
**   in 2-byte Unicode, which requires two bytes of data per character.
**
**   Note that the number of lines and columns in a text control can be
**   obtained from the control's Resource Data Record.
**
**   Write-only controls allow the control's state to be set, but the control
**   state cannot be subsequently read.  Such controls are indicated in the RDR,
**   when the WriteOnly flag is set.  SA_ERR_HPI_INVALID_CMD is returned when
**   calling this function for a write-only control.  
**
**   It is legal for both the CtrlMode parameter and the CtrlState parameter to
**   be NULL.  In this case, no data is returned other than the return code.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiControlGet (
    SAHPI_IN    SaHpiSessionIdT      SessionId,
    SAHPI_IN    SaHpiResourceIdT     ResourceId,
    SAHPI_IN    SaHpiCtrlNumT        CtrlNum,
    SAHPI_OUT   SaHpiCtrlModeT       *CtrlMode,
    SAHPI_INOUT SaHpiCtrlStateT      *CtrlState
);

/*******************************************************************************
**
** Name: saHpiControlSet()
**
** Description:
**   This function is used for setting the state and/or mode of the specified
**   control object. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   CtrlNum - [in] Control number for which the state and/or mode is being set.
**   CtrlMode - [in] The mode to set on the control.  
**   CtrlState - [in] Pointer to a control state data structure holding the
**      state to be set.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      controls, as indicated by the SAHPI_CAPABILITY_CONTROL in the
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the control is not present.
**   SA_ERR_HPI_INVALID_DATA is returned when the:
**      * CtrlState->Type field is not the correct type for the control
**         identified by the CtrlNum parameter.
**      * CtrlState->StateUnion.Analog is out of range of the control record's
**         analog Min and Max values.
**      * CtrlState->StateUnion.Text.Text.DataLength, combined with the
**         CtrlState->StateUnion.Text.Line, overflows the remaining text
**         control space. 
**      * CtrlState->StateUnion.Text.Text.DataType is not set to the DataType
**         specified in the RDR.
**      * DataType specified in the RDR is SAHPI_TL_TYPE_UNICODE or 
**         SAHPI_TL_TYPE_TEXT and CtrlState->StateUnion.Text.Text.Language is
**         not set to the Language specified in the RDR.
**      * OEM control data is invalid (see remarks below).
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * CtrlMode is not one of the valid enumerated values for this type.
**      * CtrlMode parameter is not SAHPI_CTRL_MODE_AUTO and the CtrlState
**         pointer is passed in as NULL.
**      * CtrlState->StateUnion.Digital is not one of the valid enumerated
**         values for this type.
**      * CtrlState->StateUnion.Stream.StreamLength is bigger than 
**         SAHPI_CTRL_MAX_STREAM_LENGTH.
**      * SaHpiTextBufferT structure passed as CtrlState->StateUnion.Text.Text
**         contains text characters that are not allowed according to the value
**         of CtrlState->StateUnion.Text.Text.DataType.
**   SA_ERR_HPI_INVALID_REQUEST is returned when SAHPI_CTRL_STATE_PULSE_ON is
**      issued to a digital control, which is ON (in either manual or auto 
**      mode).  It is also returned when SAHPI_CTRL_STATE_PULSE_OFF is issued
**      to a digital control, which is OFF (in either manual or auto mode).
**   SA_ERR_HPI_READ_ONLY is returned when attempting to change the mode of a
**      control with a read-only mode.
**   
** Remarks:
**   When the mode is set to SAHPI_CTRL_MODE_AUTO, the state input is ignored.
**   Ignored state inputs are not checked by the implementation.
**
**   The CtrlState parameter must be of the correct type for the specified
**   control.
**
**   If the CtrlMode parameter is set to SAHPI_CTRL_MODE_AUTO, then the
**   CtrlState parameter is not evaluated, and may be set to any value by an
**   HPI User, including a NULL pointer. Text controls include a line number
**   and a line of text in the CtrlState parameter, allowing update of just a
**   single line of a text control. The first line of the text control is line
**   number "1".  If less than a full line of data is written, the control will
**   clear all spaces beyond those written on the line. Thus writing a 
**   zero-length string will clear the addressed line. It is also possible to
**   include more characters in the text passed in the CtrlState structure than
**   will fit on one line; in this case, the control will wrap to the next line
**   (still clearing the trailing characters on the last line written). Thus,
**   there are two ways to write multiple lines to a text control: (a) call
**   saHpiControlSet() repeatedly for each line, or (b) call saHpiControlSet()
**   once and send more characters than will fit on one line. An HPI User
**   should not assume any "cursor positioning" characters are available to
**   use, but rather should always write full lines and allow "wrapping" to
**   occur. When calling saHpiControlSet() for a text control, an HPI User may
**   set the line number to SAHPI_TLN_ALL_LINES; in this case, the entire
**   control will be cleared, and the data will be written starting on line 1.
**   (This is different from simply writing at line 1, which only alters the
**   lines written to.)
**
**   This feature may be used to clear the entire control, which can be
**   accomplished by setting:
**      CtrlState->StateUnion.Text.Line = SAHPI_TLN_ALL_LINES;
**      CtrlState->StateUnion.Text.Text.DataLength = 0;
**
**   Note that the number of lines and columns in a text control can be
**   obtained from the control's RDR.
**
**   The ManufacturerId (MId) field of an OEM control is ignored by the
**   implementation when calling saHpiControlSet().  
**
**   On an OEM control, it is up to the implementation to determine what is
**   invalid data, and to return the specified error code.  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiControlSet (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiResourceIdT     ResourceId,
    SAHPI_IN SaHpiCtrlNumT        CtrlNum,
    SAHPI_IN SaHpiCtrlModeT       CtrlMode,
    SAHPI_IN SaHpiCtrlStateT      *CtrlState
);

/*******************************************************************************
**
** Name: saHpiIdrInfoGet()
**
** Description:
**   This function returns the Inventory Data Repository information including
**   the number of areas contained within the IDR and the update counter.  The
**   Inventory Data Repository is associated with a specific entity.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   IdrInfo - [out] Pointer to the information describing the requested
**      Inventory Data Repository.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the IDR is not present.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the IdrInfo pointer is passed in
**      as NULL.
**   
** Remarks:
**   The update counter provides a means for insuring that no additions or
**   changes are missed when retrieving the IDR data.  In order to use this
**   feature, an HPI User should invoke the saHpiIdrInfoGet(), and retrieve the
**   update counter value before retrieving the first Area.  After retrieving
**   all Areas and Fields of the IDR, an HPI User should again invoke the
**   saHpiIdrInfoGet() to get the update counter value.  If the update counter
**   value has not been incremented, no modification or additions were made to
**   the IDR during data retrieval.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrInfoGet( 
    SAHPI_IN SaHpiSessionIdT         SessionId,
    SAHPI_IN SaHpiResourceIdT        ResourceId,
    SAHPI_IN SaHpiIdrIdT             IdrId,
    SAHPI_OUT SaHpiIdrInfoT          *IdrInfo
);

/*******************************************************************************
**
** Name: saHpiIdrAreaHeaderGet()
**
** Description:
**   This function returns the Inventory Data Area header information for a
**   specific area associated with a particular Inventory Data Repository.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**     saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   AreaType - [in] Type of Inventory Data Area.
**   AreaId - [in] Identifier of Area entry to retrieve from the IDR.  Reserved
**      AreaId values:
**      * SAHPI_FIRST_ENTRY    Get first entry.
**      * SAHPI_LAST_ENTRY     Reserved as a delimiter for end of list.  Not a
**         valid AreaId.
**   NextAreaId - [out] Pointer to location to store the AreaId of next area of
**      the requested type within the IDR.
**   Header - [out] Pointer to Inventory Data Area Header into which the header
**      information will be placed.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by 
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * IDR is not present.
**      * AreaType parameter is set to SAHPI_IDR_AREATYPE_UNSPECIFIED, and the
**         area specified by the AreaId parameter does not exist in the IDR.
**      * AreaType parameter is set to a specific area type, but an area
**         matching both the AreaId parameter and AreaType parameter does not
**         exist.
**   SA_ERR_HPI_INVALID_PARAMS is returned if:
**      * AreaType is not one of the valid enumerated values for this type.
**      * The AreaId is an invalid reserved value such as SAHPI_LAST_ENTRY.
**      * The NextAreaId pointer is passed in as NULL.
**      * The Header pointer is passed in as NULL.
**   
** Remarks:
**   This function allows retrieval of an Inventory Data Area Header by one of
**   two ways: by AreaId regardless of type or by AreaType and AreaId.
**
**   To retrieve all areas contained within an IDR, set the AreaType parameter
**   to SAHPI_IDR_AREATYPE_UNSPECIFIED, and set the AreaId parameter to 
**   SAHPI_FIRST_ENTRY for the first call.  For each subsequent call, set the
**   AreaId parameter to the value returned in the NextAreaId parameter.
**   Continue calling this function until the NextAreaId parameter contains the
**   value SAHPI_LAST_ENTRY.
**
**   To retrieve areas of specific type within an IDR, set the AreaType
**   parameter to a valid AreaType enumeration.  Use the AreaId parameter in
**   the same manner described above to retrieve all areas of the specified
**   type.  Set the AreaId parameter to SAHPI_FIRST_ENTRY to retrieve the first
**   area of that type.  Use the value returned in NextAreaId to retrieve the
**   remaining areas of that type until SAHPI_LAST_ENTRY is returned.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrAreaHeaderGet( 
    SAHPI_IN SaHpiSessionIdT          SessionId,
    SAHPI_IN SaHpiResourceIdT         ResourceId,
    SAHPI_IN SaHpiIdrIdT              IdrId,
    SAHPI_IN SaHpiIdrAreaTypeT        AreaType,
    SAHPI_IN SaHpiEntryIdT            AreaId,
    SAHPI_OUT SaHpiEntryIdT           *NextAreaId,
    SAHPI_OUT SaHpiIdrAreaHeaderT     *Header
);

/*******************************************************************************
**
** Name: saHpiIdrAreaAdd()
**
** Description:
**   This function is used to add an Area to the specified Inventory Data
**   Repository.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   AreaType - [in] Type of Area to add.
**   AreaId- [out] Pointer to location to store the AreaId of the newly created
**      area.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**     returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the IDR is not present.
**   SA_ERR_HPI_INVALID_DATA is returned when attempting to add an area with an
**      AreaType of SAHPI_IDR_AREATYPE_UNSPECIFIED or when adding an area that
**      does not meet the implementation-specific restrictions.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if the IDR does not have enough free
**      space to allow the addition of the area.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the: 
**      * AreaId pointer is passed in as NULL.
**      * AreaType is not one of the valid enumerated values for this type.
**   SA_ERR_HPI_READ_ONLY is returned if the IDR is read-only and does not
**      allow the addition of the area.
**   
** Remarks:
**   On successful completion, the AreaId parameter will contain the AreaId of
**   the newly created area.
**
**   On successful completion, the ReadOnly element in the new Area's header
**   will always be False.
**
**   SAHPI_IDR_AREATYPE_UNSPECIFIED is not a valid area type, and should not be
**   used with this function.  If SAHPI_IDR_AREATYPE_UNSPECIFIED is specified
**   as the area type, an error code of SA_ERR_HPI_INVALID_DATA is returned.
**   This area type is only valid when used with the saHpiIdrAreaHeaderGet()
**   function to retrieve areas of an unspecified type.
**
**   Some implementations may restrict the types of areas that may be added.
**   These restrictions should be documented.  SA_ERR_HPI_INVALID_DATA is
**   returned when attempting to add an invalid area type.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrAreaAdd(
    SAHPI_IN SaHpiSessionIdT          SessionId,
    SAHPI_IN SaHpiResourceIdT         ResourceId,
    SAHPI_IN SaHpiIdrIdT              IdrId,
    SAHPI_IN SaHpiIdrAreaTypeT        AreaType,
    SAHPI_OUT SaHpiEntryIdT           *AreaId
);

/*******************************************************************************
**
** Name: saHpiIdrAreaDelete()
**
** Description:
**   This function is used to delete an Inventory Data Area, including the Area
**   header and all fields contained with the area, from a particular Inventory
**   Data Repository.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   AreaId - [in] Identifier of Area entry to delete from the IDR.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * IDR is not present.
**      * Area identified by the AreaId parameter does not exist within the IDR.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the AreaId is an invalid
**      reserved value such as SAHPI_LAST_ENTRY.
**   SA_ERR_HPI_READ_ONLY is returned if the:
**      * IDA is read-only and therefore cannot be deleted.
**      * IDA contains a read-only Field and therefore cannot be deleted.
**      * IDR is read-only as deletions are not permitted for an area from a 
**         read-only IDR.
**   
** Remarks:
**   Deleting an Inventory Data Area also deletes all fields contained within
**   the area.  
**
**   In some implementations, certain Areas are intrinsically read-only.  The 
**   ReadOnly flag, in the area header, indicates if the Area is read-only.
**
**   If the Inventory Data Area is not read-only, but contains a Field that is
**   read-only, the Area cannot be deleted.  An attempt to delete an Area that
**   contains a read-only Field will return an error.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrAreaDelete( 
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId,
    SAHPI_IN SaHpiIdrIdT            IdrId,
    SAHPI_IN SaHpiEntryIdT          AreaId
);

/*******************************************************************************
**
** Name: saHpiIdrFieldGet()
**
** Description:
**   This function returns the Inventory Data Field information from a
**   particular Inventory Data Area and Repository.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   AreaId - [in] Area identifier for the IDA.
**   FieldType - [in] Type of Inventory Data Field.
**   FieldId - [in] Identifier of Field to retrieve from the IDA.  Reserved
**      FieldId values: 
**      * SAHPI_FIRST_ENTRY     Get first entry.
**      * SAHPI_LAST_ENTRY      Reserved as a delimiter for end of list.  Not
**         a valid FieldId.
**   NextFieldId - [out] Pointer to location to store the FieldId of next field
**      of the requested type in the IDA.
**   Field - [out] Pointer to Inventory Data Field into which the field
**      information will be placed.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * IDR is not present.
**      * Area identified by AreaId is not present.
**      * FieldType parameter is set to SAHPI_IDR_FIELDTYPE_UNSPECIFIED, and
**         the field specified by the FieldId parameter does not exist in the
**         IDA.
**      * FieldType parameter is set to a specific field type, but a field
**         matching both the FieldId parameter and FieldType parameter does not
**         exist.
**   SA_ERR_HPI_INVALID_PARAMS is returned if:
**      * FieldType is not one of the valid enumerated values for this type.
**      * The AreaId or FieldId is an invalid reserved value such as
**         SAHPI_LAST_ENTRY.
**      * The NextFieldId pointer is passed in as NULL.
**      * The Field pointer is passed in as NULL.
**   
** Remarks:
**   This function allows retrieval of an Inventory Data Field by one of two
**   ways: by FieldId regardless of type or by FieldType and FieldId.  
**
**   To retrieve all fields contained within an IDA, set the FieldType parameter
**   to SAHPI_IDR_FIELDTYPE_UNSPECIFIED, and set the FieldId parameter to
**   SAHPI_FIRST_ENTRY for the first call.  For each subsequent call, set the
**   FieldId parameter to the value returned in the NextFieldId parameter.
**   Continue calling this function until the NextFieldId parameter contains the
**   value SAHPI_LAST_ENTRY.
**
**   To retrieve fields of a specific type within an IDA, set the FieldType
**   parameter to a valid Field type enumeration.  Use the FieldId parameter in
**   the same manner described above to retrieve all fields of the specified
**   type.  Set the FieldId parameter to SAHPI_FIRST_ENTRY to retrieve the first
**   field of that type.  Use the value returned in NextFieldId to retrieve the
**   remaining fields of that type until SAHPI_LAST_ENTRY is returned.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrFieldGet( 
    SAHPI_IN SaHpiSessionIdT         SessionId,
    SAHPI_IN SaHpiResourceIdT        ResourceId,
    SAHPI_IN SaHpiIdrIdT             IdrId,
    SAHPI_IN SaHpiEntryIdT           AreaId,
    SAHPI_IN SaHpiIdrFieldTypeT      FieldType,
    SAHPI_IN SaHpiEntryIdT           FieldId,
    SAHPI_OUT SaHpiEntryIdT          *NextFieldId,
    SAHPI_OUT SaHpiIdrFieldT         *Field
);

/*******************************************************************************
**
** Name: saHpiIdrFieldAdd()
**
** Description:
**   This function is used to add a field to the specified Inventory Data Area
**   with a specific Inventory Data Repository.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   Field - [in/out] Pointer to Inventory Data Field, which contains the new
**      field information to add to the IDA.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * IDR is not present.
**      * Area identified by Field?AreaId does not exist within the IDR.
**   SA_ERR_HPI_INVALID_DATA is returned if the Field data is incorrectly
**      formatted or does not meet the restrictions for the implementation.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if the IDR does not have enough free
**      space to allow the addition of the field.
**   SA_ERR_HPI_READ_ONLY is returned if the:
**      * Area identified by Field?AreaId is read-only and does not allow
**         modification to its fields.
**      * IDR is identified by IdrId, is read-only, and does not allow
**         modification to its fields.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * The Field type is not one of the valid field type enumerated values.
**      * Field type, Field?Type, is set to SAHPI_IDR_FIELDTYPE_UNSPECIFIED.
**      * SaHpiTextBufferT structure passed as part of the Field parameter is
**         not valid.  This occurs when:
**         * The DataType is not one of the enumerated values for that type, or
**         * The data field contains characters that are not legal according to
**            the value of DataType, or 
**         * The Language is not one of the enumerated values for that type
**            when the DataType is SAHPI_TL_TYPE_UNICODE or SAHPI_TL_TYPE_TEXT.
**       * Field pointer is passed in as NULL.
**   
** Remarks:
**   The FieldId element within the Field parameter is not used by this
**   function.  Instead, on successful completion, the FieldId field is set to
**   the new value associated with the added Field.
**
**   The ReadOnly element in the Field parameter is not used by this function.
**   On successful completion, the ReadOnly element in the new Field will
**   always be False.
**
**   Addition of a read-only Inventory Data Field is not allowed; therefore the
**   ReadOnly element in the SaHpiIdrFieldT parameter is ignored.
**
**   SAHPI_IDR_FIELDTYPE_UNSPECIFIED is not a valid field type, and should not
**   be used with this function.  If SAHPI_IDR_FIELDTYPE_UNSPECIFIED is
**   specified as the field type, an error code of SA_ERR_HPI_INVALID_DATA is
**   returned.  This field type is only valid when used with the
**   saHpiIdrFieldGet() function to retrieve fields of an unspecified type.
**
**   Some implementations have restrictions on what fields are valid in specific
**   areas and/or the data format of some fields.  These restrictions should be
**   documented.  SA_ERR_HPI_INVALID_DATA is returned when the field type or
**   field data does not meet the implementation-specific restrictions.
**
**   The AreaId element within the Field parameter identifies the specific IDA
**   into which the new field is added.  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrFieldAdd( 
    SAHPI_IN SaHpiSessionIdT          SessionId,
    SAHPI_IN SaHpiResourceIdT         ResourceId,
    SAHPI_IN SaHpiIdrIdT              IdrId,
    SAHPI_INOUT SaHpiIdrFieldT        *Field
);

/*******************************************************************************
**
** Name: saHpiIdrFieldSet()
**
** Description:
**   This function is used to update an Inventory Data Field for a particular
**   Inventory Data Area and Repository.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   Field - [in] Pointer to Inventory Data Field, which contains the updated
**      field information.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * IDR is not present.
**      * Area identified by Field?AreaId does not exist within the IDR or if
**         the Field does not exist within the Inventory Data Area.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the:
**      * Field pointer is passed in as NULL.
**      * Field type, Field?Type,  is not set to one of the valid field type
**         enumerated values.
**      * Field type, Field?Type, is set to SAHPI_IDR_FIELDTYPE_UNSPECIFIED.
**   SA_ERR_HPI_INVALID_DATA is returned if the field data is incorrectly
**      formatted or does not meet the restrictions for the implementation.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if the IDR does not have enough free
**      space to allow the modification of the field due to an increase in the
**      field size.
**   SA_ERR_HPI_READ_ONLY is returned if the:
**      * Field is read-only and does not allow modifications.
**      * Area is read-only and does not allow modifications to its fields.
**      * IDR is read-only and does not allow modifications to its fields.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the SaHpiTextBufferT structure
**      passed as part of the Field parameter is not valid.  This occurs when:
**      * The DataType is not one of the enumerated values for that type, or 
**      * The data field contains characters that are not legal according to
**         the value of DataType, or
**      * The Language is not one of the enumerated values for that type when
**         the DataType is SAHPI_TL_TYPE_UNICODE or SAHPI_TL_TYPE_TEXT.
**   
** Remarks:
**   This function allows modification of both the FieldType and Field data of
**   a given Inventory Data Field.  This function does not allow modification of
**   the read-only attribute of the Inventory Data Field; therefore after a
**   successful update, the ReadOnly element in the updated Field will always be
**   False. The input value for ReadOnly is ignored.
**
**   SAHPI_IDR_FIELDTYPE_UNSPECIFIED is not a valid field type, and should not
**   be used with this function.  If SAHPI_IDR_FIELDTYPE_UNSPECIFIED is
**   specified as the new field type, an error code of SA_ERR_HPI_INVALID_DATA
**   is returned.  This field type is only valid when used with the
**   saHpiIdrFieldGet() function to retrieve fields of an unspecified type.
**
**   Some implementations have restrictions on what fields are valid in specific
**   areas and/or the data format of some fields.  These restrictions should be
**   documented.  SA_ERR_HPI_INVALID_DATA is returned when the field type or
**   field data does not meet the implementation-specific restrictions. 
**
**   In some implementations, certain Fields are intrinsically read-only.  The
**   ReadOnly flag, in the field structure, indicates if the Field is read-only.
**
**   The Field to update is identified by the AreaId and FieldId elements within
**   the Field parameter. 
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrFieldSet( 
    SAHPI_IN SaHpiSessionIdT          SessionId,
    SAHPI_IN SaHpiResourceIdT         ResourceId,
    SAHPI_IN SaHpiIdrIdT              IdrId,
    SAHPI_IN SaHpiIdrFieldT           *Field
);

/*******************************************************************************
**
** Name: saHpiIdrFieldDelete()
**
** Description:
**   This function is used to delete an Inventory Data Field from a particular
**   Inventory Data Area and Repository.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   IdrId - [in] Identifier for the Inventory Data Repository.
**   AreaId - [in] Area identifier for the IDA.
**   FieldId - [in] Identifier of Field to delete from the IDA.  
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support an
**      inventory data repository, as indicated by
**      SAHPI_CAPABILITY_INVENTORY_DATA in the resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the:
**      * IDR is not present.
**      * Area identified by the AreaId parameter does not exist within the IDR,
**         or if the Field identified by the FieldId parameter does not exist
**         within the IDA.
**   SA_ERR_HPI_READ_ONLY is returned if the field, the IDA, or the IDR is
**      read-only and does not allow deletion.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the AreaId or FieldId is an
**      invalid reserved value such as SAHPI_LAST_ENTRY.
**   
** Remarks:
**   In some implementations, certain fields are intrinsically read-only. The
**   ReadOnly flag, in the field structure, indicates if the field is read-only.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiIdrFieldDelete( 
    SAHPI_IN SaHpiSessionIdT          SessionId,
    SAHPI_IN SaHpiResourceIdT         ResourceId,
    SAHPI_IN SaHpiIdrIdT              IdrId,
    SAHPI_IN SaHpiEntryIdT            AreaId,
    SAHPI_IN SaHpiEntryIdT            FieldId
);

/*******************************************************************************
**
** Name: saHpiWatchdogTimerGet()
**
** Description:
**   This function retrieves the current watchdog timer settings and
**   configuration. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   WatchdogNum - [in] Watchdog number that specifies the watchdog timer on a
**      resource.
**   Watchdog - [out] Pointer to watchdog data structure.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support a 
**      watchdog timer, as indicated by SAHPI_CAPABILITY_WATCHDOG in the 
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the WatchdogNum is not present.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Watchdog pointer is passed
**      in as NULL.
**   
** Remarks:
**   See the description of the SaHpiWatchdogT structure in Section 8.11 on
**   page 180 for details on what information is returned by this function.
**
**   When the watchdog action is SAHPI_WA_RESET, the type of reset (warm or
**   cold) is implementation-dependent.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiWatchdogTimerGet (
    SAHPI_IN  SaHpiSessionIdT        SessionId,
    SAHPI_IN  SaHpiResourceIdT       ResourceId,
    SAHPI_IN  SaHpiWatchdogNumT      WatchdogNum,
    SAHPI_OUT SaHpiWatchdogT         *Watchdog
);

/*******************************************************************************
**
** Name: saHpiWatchdogTimerSet()
**
** Description:
**   This function provides a method for initializing the watchdog timer
**   configuration.
**
**   Once the appropriate configuration has been set using
**   saHpiWatchdogTimerSet(), an HPI User must then call
**   saHpiWatchdogTimerReset() to initially start the watchdog timer, unless
**   the watchdog timer was already running prior to calling
**   saHpiWatchdogTimerSet() and the Running field in the passed Watchdog
**   structure is True. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   WatchdogNum - [in] The watchdog number specifying the specific watchdog
**      timer on a resource.
**   Watchdog - [in] Pointer to watchdog data structure.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support a
**      watchdog timer, as indicated by SAHPI_CAPABILITY_WATCHDOG in the 
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the WatchdogNum is not present.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the:
**      * Watchdog->TimerUse is not one of the valid enumerated values for this
**         type.
**      * Watchdog->TimerAction is not one of the valid enumerated values for
**         this type.
**      * Watchdog->PretimerInterrupt is not one of the valid enumerated values
**         for this type.
**      * Watchdog pointer is passed in as NULL.
**   SA_ERR_HPI_INVALID_DATA is returned when the:
**      * Watchdog->PreTimeoutInterval is outside the acceptable range for the
**         implementation.
**      * Watchdog->InitialCount is outside the acceptable range for the
**         implementation.
**      * Value of Watchdog->PreTimeoutInterval is greater than the value of
**         Watchdog->InitialCount.
**      * Watchdog->PretimerInterrupt is set to an unsupported value.  See 
**         remarks.
**      * Watchdog->TimerAction is set to an unsupported value.  See remarks.
**      * Watchdog->TimerUse is set to an unsupported value.  See remarks.
**   
** Remarks:
**   Configuring the watchdog timer with the saHpiWatchdogTimerSet() function
**   does not start the timer running.  If the timer was previously stopped
**   when this function is called, then it will remain stopped, and must be
**   started by calling saHpiWatchdogTimerReset().  If the timer was previously
**   running, then it will continue to run if the Watchdog->Running field passed
**   is True, or will be stopped if the Watchdog->Running field passed is False.
**   If it continues to run, it will restart its count-down from the newly
**   configured initial count value.
**
**   If the initial counter value is set to 0, then any configured pre-timer
**   interrupt action or watchdog timer expiration action will be taken
**   immediately when the watchdog timer is started.  This provides a mechanism
**   for software to force an immediate recovery action should that be dependent
**   on a Watchdog timeout occurring.
**
**   See the description of the SaHpiWatchdogT structure for more details on the
**   effects of this command related to specific data passed in that structure.
**
**   Some implementations impose a limit on the acceptable ranges for the
**   PreTimeoutInterval or InitialCount. These limitations must be documented. 
**   SA_ERR_HPI_INVALID_DATA is returned if an attempt is made to set a value
**   outside of the supported range.
**
**   Some implementations cannot accept all of the enumerated values for
**   TimerUse, TimerAction, or PretimerInterrupt.  These restrictions should be
**   documented.  SA_ERR_HPI_INVALID_DATA is returned if an attempt is made to
**   select an unsupported option.
**
**   When the watchdog action is set to SAHPI_WA_RESET, the type of reset (warm
**   or cold) is implementation-dependent.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiWatchdogTimerSet (
    SAHPI_IN SaHpiSessionIdT       SessionId,
    SAHPI_IN SaHpiResourceIdT      ResourceId,
    SAHPI_IN SaHpiWatchdogNumT     WatchdogNum,
    SAHPI_IN SaHpiWatchdogT        *Watchdog
);

/*******************************************************************************
**
** Name: saHpiWatchdogTimerReset()
**
** Description:
**   This function provides a method to start or restart the watchdog timer from
**   the initial countdown value.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   WatchdogNum - [in] The watchdog number specifying the specific watchdog
**      timer on a resource.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support a
**      watchdog timer, as indicated by SAHPI_CAPABILITY_WATCHDOG in the 
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the WatchdogNum is not present.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the current watchdog state does
**      not allow a reset.
**   
** Remarks:
**   If the Watchdog has been configured to issue a Pre-Timeout interrupt, and
**   that interrupt has already occurred, the saHpiWatchdogTimerReset() function
**   will not reset  the watchdog counter. The only way to stop a Watchdog from
**   timing out once a Pre-Timeout interrupt has occurred is to use the 
**   saHpiWatchdogTimerSet() function to reset and/or stop the timer. 
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiWatchdogTimerReset (
    SAHPI_IN SaHpiSessionIdT       SessionId,
    SAHPI_IN SaHpiResourceIdT      ResourceId,
    SAHPI_IN SaHpiWatchdogNumT     WatchdogNum
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorGetNext()
**
** Description:
**   This function allows retrieval of an announcement from the current set of
**   announcements held in the Annunciator.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   Severity - [in] Severity level of announcements to retrieve.  Set to 
**      SAHPI_ALL_SEVERITIES to retrieve announcement of any severity;
**      otherwise, set to requested severity level.
**   UnacknowledgedOnly - [in] Set to True to indicate only unacknowledged
**      announcements should be returned.  Set to False to indicate either an
**      acknowledged or unacknowledged announcement may be returned.
**   Announcement  - [in/out] Pointer to the structure to hold the returned
**      announcement. Also, on input, Announcement->EntryId and 
**      Announcement->Timestamp are used to identify the "previous" 
**      announcement.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when Severity is not one of the
**      valid enumerated values for this type.
**   SA_ERR_HPI_NOT_PRESENT is returned if:
**      * The AnnunciatorNum passed does not address a valid Annunciator
**         supported by the resource. 
**      * There are no announcements (or no unacknowledged announcements if
**         UnacknowledgedOnly is True) that meet the specified Severity in the
**         current set after the announcement identified by
**         Announcement->EntryId and Announcement->Timestamp.
**      * There are no announcements (or no unacknowledged announcements if 
**         UnacknowledgedOnly is True) that meet the specified Severity in the
**         current set if the passed Announcement->EntryId field was set to 
**         SAHPI_FIRST_ENTRY.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Announcement parameter is
**      passed in as NULL.
**   SA_ERR_HPI_INVALID_DATA is returned if the passed Announcement->EntryId
**   matches an announcement in the current set, but the passed 
**   Announcement->Timestamp does not match the timestamp of that announcement.
**   
** Remarks:
**   All announcements contained in the set are maintained in the order in which
**   they were added.  This function will return the next announcement meeting
**   the specifications given by an HPI User that was added to the set after the
**   announcement whose EntryId and timestamp is passed by an HPI User.   If 
**   SAHPI_FIRST_ENTRY is passed as the EntryId, the first announcement in the
**   set meeting the specifications given by an HPI User is returned.  This
**   function should operate even if the announcement associated with the
**   EntryId and Timestamp passed by an HPI User has been deleted.
**
**   Selection can be restricted to only announcements of a specified severity,
**   and/or only unacknowledged announcements. To retrieve all announcements
**   contained meeting specific requirements, call saHpiAnnunciatorGetNext()
**   with the Status->EntryId field set to SAHPI_FIRST_ENTRY and the Severity
**   and UnacknowledgedOnly parameters set to select what announcements should
**   be returned.  Then, repeatedly call saHpiAnnunciatorGetNext() passing the
**   previously returned announcement as the Announcement parameter, and the
**   same values for Severity and UnacknowledgedOnly until the function returns
**   with the error code SA_ERR_HPI_NOT_PRESENT.
**
**   SAHPI_FIRST_ENTRY and SAHPI_LAST_ENTRY are reserved EntryId values, and
**   will never be assigned to an announcement.
**  
**   The elements EntryId and Timestamp are used in the Announcement parameter
**   to identify the "previous" announcement; the next announcement added to the
**   table after this announcement that meets the Severity and
**   UnacknowledgedOnly requirements will be returned.  Announcement->EntryId
**   may be set to SAHPI_FIRST_ENTRY to select the first announcement in the
**   current set meeting the Severity and UnacknowledgedOnly requirements.  If
**   Announcement->EntryId is SAHPI_FIRST_ENTRY, then    Announcement->Timestamp
**   is ignored.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorGetNext( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_IN SaHpiSeverityT             Severity,
    SAHPI_IN SaHpiBoolT                 UnacknowledgedOnly,
    SAHPI_INOUT SaHpiAnnouncementT      *Announcement
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorGet()
**
** Description:
**   This function allows retrieval of a specific announcement in the
**   Annunciator's current set by referencing its EntryId.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained
**      using saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   EntryId - [in] Identifier of the announcement to retrieve from the
**      Annunciator.
**   Announcement - [out] Pointer to the structure to hold the returned
**      announcement. 
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if:
**      * The AnnunciatorNum passed does not address a valid Annunciator
**         supported by the resource.
**      * The requested EntryId does not correspond to an announcement
**         contained in the Annunciator.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Announcement parameter is
**      passed in as NULL.
**   
** Remarks:
**   SAHPI_FIRST_ENTRY and SAHPI_LAST_ENTRY are reserved EntryId values, and
**   will never be assigned to announcements.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorGet( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_IN SaHpiEntryIdT              EntryId,
    SAHPI_OUT SaHpiAnnouncementT        *Announcement
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorAcknowledge()
**
** Description:
**   This function allows an HPI User to acknowledge a single announcement or a
**   group of announcements by severity.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   EntryId - [in] Entry identifier of the announcement to acknowledge.
**      Reserved EntryId values:
**      * SAHPI_ENTRY_UNSPECIFIED     Ignore this parameter.
**   Severity - [in] Severity level of announcements to acknowledge.  Ignored
**      unless EntryId is SAHPI_ENTRY_UNSPECIFIED.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**     resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if:
**      * The AnnunciatorNum passed does not address a valid Annunciator
**         supported by the resource.
**      * An announcement identified by the EntryId parameter does not exist in
**         the current set.
**   SA_ERR_HPI_INVALID_PARAMS is returned if EntryId is SAHPI_ENTRY_UNSPECIFIED
**      and Severity is not one of the valid enumerated values for this type.
**   
** Remarks:
**   Announcements are acknowledged by one of two ways: a single announcement
**   by EntryId regardless of severity or as a group of announcements by
**   severity regardless of EntryId.  
**
**   An HPI User acknowledges an announcement to influence the platform-specific
**   annunciation provided by the Annunciator management instrument. 
** 
**   An acknowledged announcement will have the Acknowledged field set to True.
**
**   To acknowledge all announcements contained within the current set, set the
**   Severity parameter to SAHPI_ALL_SEVERITIES, and set the EntryId parameter
**   to SAHPI_ENTRY_UNSPECIFIED.
**
**   To acknowledge all announcements of a specific severity, set the Severity
**   parameter to the appropriate value, and set the EntryId parameter to
**   SAHPI_ENTRY_UNSPECIFIED.
**
**   To acknowledge a single announcement, set the EntryId parameter to a value
**   other than SAHPI_ENTRY_UNSPECIFIED.  The EntryId must be a valid identifier
**   for an announcement present in the current set.
**
**   If an announcement has been previously acknowledged, acknowledging it again
**   has no effect.  However, this is not an error.
**
**   If the EntryId parameter has a value other than SAHPI_ENTRY_UNSPECIFIED,
**   the Severity parameter is ignored.
**
**   If the EntryId parameter is passed as SAHPI_ENTRY_UNSPECIFIED, and no
**   announcements are present that meet the requested Severity, this function
**   will have no effect.  However, this is not an error.
**
**   SAHPI_ENTRY_UNSPECIFIED is defined as the same value as SAHPI_FIRST_ENTRY,
**   so using either symbol will have the same effect.  However,
**   SAHPI_ENTRY_UNSPECIFIED should be used with this function for clarity.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorAcknowledge( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_IN SaHpiEntryIdT              EntryId,
    SAHPI_IN SaHpiSeverityT             Severity
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorAdd()
**
** Description:
**   This function is used to add an announcement to the set of items held by an
**   Annunciator management instrument.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   Announcement - [in/out] Pointer to structure that contains the new
**      announcement to add to the set.  
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the AnnunciatorNum passed does not
**      address a valid Annunciator supported by the resource.
**   SA_ERR_HPI_INVALID_PARAMS is returned when:
**      * The Announcement pointer is passed in as NULL.
**      * The Announcement->Severity passed is not valid.
**      * The Announcement->StatusCond structure passed in is not valid.
**   SA_ERR_HPI_OUT_OF_SPACE is returned if the Annunciator is not able to add
**      an additional announcement due to resource limits.
**   SA_ERR_HPI_READ_ONLY is returned if the Annunciator is in auto mode.
**   
** Remarks:
**   The EntryId, Timestamp, and AddedByUser fields within the Announcement
**   parameter are not used by this function.  Instead, on successful
**   completion, these fields are set to new values associated with the added
**   announcement.  AddedByUser will always be set to True.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorAdd( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_INOUT SaHpiAnnouncementT      *Announcement
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorDelete()
**
** Description:
**   This function allows an HPI User to delete a single announcement or a group
**   of announcements from the current set of an Annunciator.  Announcements may
**   be deleted individually by specifying a specific EntryId, or they may be
**   deleted as a group by specifying a severity.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   EntryId - [in] Entry identifier of the announcement to delete.  Reserved
**      EntryId values:
**      * SAHPI_ENTRY_UNSPECIFIED     Ignore this parameter.
**   Severity - [in] Severity level of announcements to delete.  Ignored unless
**      EntryId is SAHPI_ENTRY_UNSPECIFIED.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned if EntryId is SAHPI_ENTRY_UNSPECIFIED
**      and Severity is not one of the valid enumerated values for this type.
**   SA_ERR_HPI_NOT_PRESENT is returned if:
**      * The AnnunciatorNum passed does not address a valid Annunciator
**         supported by the resource 
**      * An announcement identified by the EntryId parameter does not exist in
**         the current set of the Annunciator.
**   SA_ERR_HPI_READ_ONLY is returned if the Annunciator is in auto mode.
**   
** Remarks:
**   To delete a single, specific announcement, set the EntryId parameter to a
**   value representing an actual announcement in the current set.  The Severity
**   parameter is ignored when the EntryId parameter is set to a value other
**   than SAHPI_ENTRY_UNSPECIFIED.
**
**   To delete a group of announcements, set the EntryId parameter to 
**   SAHPI_ENTRY_UNSPECIFIED, and set the Severity parameter to identify which
**   severity of announcements should be deleted.  To clear all announcements
**   contained within the Annunciator, set the Severity parameter to
**   SAHPI_ALL_SEVERITIES. 
**
**   If the EntryId parameter is passed as SAHPI_ENTRY_UNSPECIFIED, and no
**   announcements are present that meet the specified Severity, this function
**   will have no effect.  However, this is not an error.
**
**   SAHPI_ENTRY_UNSPECIFIED is defined as the same value as SAHPI_FIRST_ENTRY,
**   so using either symbol will have the same effect.  However,
**   SAHPI_ENTRY_UNSPECIFIED should be used with this function for clarity.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorDelete( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_IN SaHpiEntryIdT              EntryId,
    SAHPI_IN SaHpiSeverityT             Severity
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorModeGet()
**
** Description:
**   This function allows an HPI User to retrieve the current operating mode of
**   an Annunciator. The mode indicates whether or not an HPI User is allowed to
**   add or delete items in the set, and whether or not the HPI implementation
**   will automatically add or delete items in the set. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   Mode - [out] Pointer to location to store the current operating mode of the
**      Annunciator where the returned value will be one of the following:
**      * SAHPI_ANNUNCIATOR_MODE_AUTO - the HPI implementation may add or delete
**         announcements in the set; an HPI User may not add or delete
**         announcements in the set.
**      * SAHPI_ANNUNCIATOR_MODE_USER - the HPI implementation may not add or
**         delete announcements in the set; an HPI User may add or delete
**         announcements in the set.
**      * SAHPI_ANNUNCIATOR_MODE_SHARED - both the HPI implementation and an HPI
**         User may add or delete announcements in the set.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the AnnunciatorNum passed does not
**      address a valid Annunciator supported by the resource.  
**   SA_ERR_HPI_INVALID_PARAMS is returned if Mode is passed in as NULL.
**   
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorModeGet( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_OUT SaHpiAnnunciatorModeT     *Mode
);

/*******************************************************************************
**
** Name: saHpiAnnunciatorModeSet()
**
** Description:
**   This function allows an HPI User to change the current operating mode of an
**   Annunciator. The mode indicates whether or not an HPI User is allowed to
**   add or delete announcements in the set, and whether or not the HPI
**   implementation will automatically add or delete announcements in the set.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   AnnunciatorNum - [in] Annunciator number for the addressed Annunciator.
**   Mode - [out] Mode to set for the Annunciator:
**      * SAHPI_ANNUNCIATOR_MODE_AUTO - the HPI implementation may add or delete
**         announcements in the set; an HPI User may not add or delete
**         announcements in the set.
**      * SAHPI_ANNUNCIATOR_MODE_USER - the HPI implementation may not add or
**         delete announcements in the set; an HPI User may add or delete
**         announcements in the set.
**      * SAHPI_ANNUNCIATOR_MODE_SHARED - both the HPI implementation and an HPI
**         User may add or delete announcements in the set.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      Annunciators, as indicated by SAHPI_CAPABILITY_ANNUNCIATOR in the
**      resource's RPT entry.
**   SA_ERR_HPI_NOT_PRESENT is returned if the AnnunciatorNum passed does not
**      address a valid Annunciator supported by the resource.
**   SA_ERR_HPI_INVALID_PARAMS is returned if Mode is not a valid enumerated
**      value for this type.
**   SA_ERR_HPI_READ_ONLY is returned if mode changing is not permitted for
**      this Annunciator.
**   
** Remarks:
**   None.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAnnunciatorModeSet( 
    SAHPI_IN SaHpiSessionIdT            SessionId,
    SAHPI_IN SaHpiResourceIdT           ResourceId,
    SAHPI_IN SaHpiAnnunciatorNumT       AnnunciatorNum,
    SAHPI_IN SaHpiAnnunciatorModeT      Mode
);

/*******************************************************************************
**
** Name: saHpiHotSwapPolicyCancel()
**
** Description:
**   A resource supporting hot swap typically supports default policies for
**   insertion and extraction. On insertion, the default policy may be for the
**   resource to turn the associated FRU's local power on and to de-assert 
**   reset.  On extraction, the default policy may be for the resource to
**   immediately power off the FRU and turn on a hot swap indicator. This 
**   function allows an HPI User, after receiving a hot swap event with
**   HotSwapState equal to SAHPI_HS_STATE_INSERTION_PENDING or 
**   SAHPI_HS_STATE_EXTRACTION_PENDING, to prevent the default policy from
**   being executed.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support
**      managed hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in
**      the resource's RPT entry.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the resource is:
**      * Not in the INSERTION PENDING or EXTRACTION PENDING state.
**      * Processing an auto-insertion or auto-extraction policy.
**   
** Remarks:
**   Each time the resource transitions to either the INSERTION PENDING or
**   EXTRACTION PENDING state, the default policies will execute after the
**   configured timeout period, unless cancelled using 
**   saHpiHotSwapPolicyCancel() after the transition to INSERTION PENDING or
**   EXTRACTION PENDING state and before the timeout expires.The default policy
**   cannot be canceled once it has begun to execute.  
**
**   Because a resource that supports the simplified hot swap model will never
**   transition into INSERTION PENDING or EXTRACTION PENDING states, this
**   function is not applicable to those resources.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiHotSwapPolicyCancel (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiResourceIdT     ResourceId
);

/*******************************************************************************
**
** Name: saHpiResourceActiveSet()
**
** Description:
**   This function allows an HPI User to request a resource to transition to the
**   ACTIVE state from the INSERTION PENDING or EXTRACTION PENDING state.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support managed
**      hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the resource is:
**      * Not in the INSERTION PENDING or EXTRACTION PENDING state.
**      * Processing an auto-insertion or auto-extraction policy.
**   
** Remarks:
**   During insertion, a resource supporting hot swap will generate an event to
**   indicate that it is in the INSERTION PENDING state. If an HPI User calls 
**   saHpiHotSwapPolicyCancel() before the resource begins an auto-insert 
**   operation, then the resource will remain in INSERTION PENDING state while
**   an HPI User acts on the resource to integrate it into the system. During
**   this state, an HPI User can instruct the resource to power on the
**   associated FRU, to de-assert reset, or to turn off its hot swap indicator
**   using the saHpiResourcePowerStateSet(), saHpiResourceResetStateSet(), or 
**   saHpiHotSwapIndicatorStateSet() functions, respectively, if the resource
**   has those associated capabilities. Once an HPI User has completed with the
**   integration of the FRU, this function must be called to signal that the
**   resource should now transition into the ACTIVE state. 
**
**   An HPI User may also use this function to request a resource to return to
**   the ACTIVE state from the EXTRACTION PENDING state in order to reject an
**   extraction request.
**
**   Because a resource that supports the simplified hot swap model will never
**   transition into INSERTION PENDING or EXTRACTION PENDING states, this
**   function is not applicable to those resources.
**
**   Only valid if resource is in INSERTION PENDING or EXTRACTION PENDING state
**   and an auto-insert or auto-extract policy action has not been initiated.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceActiveSet (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId
);

/*******************************************************************************
**
** Name: saHpiResourceInactiveSet()
**
** Description:
**   This function allows an HPI User to request a resource to transition to the
**   INACTIVE state from the INSERTION PENDING or EXTRACTION PENDING state.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support managed
**      hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the resource is:
**      * Not in the INSERTION PENDING or EXTRACTION PENDING state.
**      * Processing an auto-insertion or auto-extraction policy.
**   
** Remarks:
**   During extraction, a resource supporting hot swap will generate an event
**   to indicate that it is in the EXTRACTION PENDING state. If an HPI User
**   calls saHpiHotSwapPolicyCancel() before the resource begins an auto-extract
**   operation, then the resource will remain in EXTRACTION PENDING state while
**   an HPI User acts on the resource to isolate the associated FRU from the
**   system. During this state, an HPI User can instruct the resource to power
**   off the FRU, to assert reset, or to turn on its hot swap indicator using
**   the saHpiResourcePowerStateSet(), saHpiResourceResetStateSet(), or 
**   saHpiHotSwapIndicatorStateSet() functions, respectively, if the resource
**   has these associated capabilities. Once an HPI User has completed the
**   shutdown of the FRU, this function must be called to signal that the
**   resource should now transition into the INACTIVE state. 
**
**   An HPI User may also use this function to request a resource to return to
**   the INACTIVE state from the INSERTION PENDING state to abort a hot-swap
**   insertion action.
**
**   Because a resource that supports the simplified hot swap model will never
**   transition into INSERTION PENDING or EXTRACTION PENDING states, this
**   function is not applicable to those resources.
**
**   Only valid if resource is in EXTRACTION PENDING or INSERTION PENDING state
**   and an auto-extract or auto-insert policy action has not been initiated.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceInactiveSet (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId
);

/*******************************************************************************
**
** Name: saHpiAutoInsertTimeoutGet()
**
** Description:
**   This function allows an HPI User to request the auto-insert timeout value
**   within a specific domain. This value indicates how long the resource will 
**   wait before the default auto-insertion policy is invoked. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   Timeout - [out] Pointer to location to store the number of nanoseconds to 
**      wait before autonomous handling of the hot swap event. Reserved time 
**      outvalues:
**      * SAHPI_TIMEOUT_IMMEDIATE indicates autonomous handling is immediate.
**      * SAHPI_TIMEOUT_BLOCK indicates autonomous handling does not occur.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_INVALID_PARAMS is returned if the Timeout pointer is passed in
**      as NULL.
**   
** Remarks:
**   Each domain maintains a single auto-insert timeout value and it is applied
**   to all contained resources upon insertion, which support managed hot swap.
**   Further information on the auto-insert timeout can be found in the
**   function saHpiAutoInsertTimeoutSet().  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAutoInsertTimeoutGet(
    SAHPI_IN  SaHpiSessionIdT     SessionId,
    SAHPI_OUT SaHpiTimeoutT       *Timeout
);

/*******************************************************************************
**
** Name: saHpiAutoInsertTimeoutSet()
**
** Description:
**   This function allows an HPI User to configure a timeout for how long to
**   wait before the default auto-insertion policy is invoked on a resource
**   within a specific domain.  
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   Timeout - [in] The number of nanoseconds to wait before autonomous handling
**      of the hot swap event.  Reserved time out values:
**      * SAHPI_TIMEOUT_IMMEDIATE indicates proceed immediately to autonomous
**         handling.
**      * SAHPI_TIMEOUT_BLOCK indicates prevent autonomous handling.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_READ_ONLY is returned if the auto-insert timeout for the domain
**      is fixed as indicated by the SAHPI_DOMAIN_CAP_AUTOINSERT_READ_ONLY flag
**      in the DomainInfo structure.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Timeout parameter is not set
**      to SAHPI_TIMEOUT_BLOCK, SAHPI_TIMEOUT_IMMEDIATE or a positive value.
**   
** Remarks:
**   This function accepts a parameter instructing each resource to impose a
**   delay before performing its default hot swap policy for auto-insertion.
**   The parameter may be set to SAHPI_TIMEOUT_IMMEDIATE to direct resources to
**   proceed immediately to auto-insertion, or to SAHPI_TIMEOUT_BLOCK to prevent
**   auto-insertion from ever occurring.  If the parameter is set to another
**   value, then it defines the number of nanoseconds between the time a hot
**   swap event with HotSwapState = SAHPI_HS_STATE_INSERTION_PENDING is
**   generated, and the time that the auto-insertion policy will be invoked for
**   that resource.  If, during this time period, a saHpiHotSwapPolicyCancel()
**   function call is processed, the timer will be stopped, and the
**   auto-insertion policy will not be invoked.  Each domain maintains a single
**   auto-insert timeout value and it is applied to all contained resources upon
**   insertion, which support managed hot swap.
**
**   Once the auto-insertion policy begins, an HPI User will not be allowed to
**   cancel the insertion policy; hence, the timeout should be set appropriately
**   to allow for this condition.  Note that the timeout period begins when the
**   hot swap event with HotSwapState = SAHPI_HS_STATE_INSERTION_PENDING is 
**   initially generated; not when it is received by an HPI User with a
**   saHpiEventGet() function call, or even when it is placed in a session
**   event queue.
**
**   A resource may exist in multiple domains, which themselves may have
**   different auto-insertion timeout values.  Upon insertion, the resource will
**   begin its auto-insertion policy based on the smallest auto-insertion
**   timeout value.  As an example, if a resource is inserted into two domains,
**   one with an auto-insertion timeout of 5 seconds, and one with an
**   auto-insertion timeout of 10 seconds, the resource will begin its
**   auto-insertion policy at 5 seconds.
**
**   An implementation may enforce a fixed, preset timeout value.  In such 
**   cases, the SAHPI_DOMAIN_CAP_AUTOINSERT_READ_ONLY flag will be set to 
**   indicate that an HPI User cannot change the auto-insert Timeout value. 
**   SA_ERR_HPI_READ_ONLY is returned if an HPI User attempts to change a 
**   read-only timeout.  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAutoInsertTimeoutSet( 
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiTimeoutT          Timeout
);

/*******************************************************************************
**
** Name: saHpiAutoExtractTimeoutGet()
**
** Description:
**   This function allows an HPI User to request the timeout for how long a
**   resource will wait before the default auto-extraction policy is invoked. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   Timeout - [out] Pointer to location to store the number of nanoseconds to
**      wait before autonomous handling of the hot swap event. Reserved time
**      out values:
**      * SAHPI_TIMEOUT_IMMEDIATE indicates autonomous handling is immediate.
**      * SAHPI_TIMEOUT_BLOCK indicates autonomous handling does not occur.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support managed
**      hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in the 
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the Timeout pointer is passed in
**      as NULL.
**   
** Remarks:
**   Further information on auto-extract timeouts is detailed in 
**   saHpiAutoExtractTimeoutSet().
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAutoExtractTimeoutGet(
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_OUT SaHpiTimeoutT        *Timeout
);

/*******************************************************************************
**
** Name: saHpiAutoExtractTimeoutSet()
**
** Description:
**   This function allows an HPI User to configure a timeout for how long to
**   wait before the default auto-extraction policy is invoked. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   Timeout - [in] The number of nanoseconds to wait before autonomous handling
**      of the hot swap event. Reserved timeout values:
**      * SAHPI_TIMEOUT_IMMEDIATE indicates proceed immediately to autonomous
**         handling.
**      * SAHPI_TIMEOUT_BLOCK indicates prevent autonomous handling.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is 
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support managed
**      hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when the Timeout parameter is not set
**      to SAHPI_TIMEOUT_BLOCK, SAHPI_TIMEOUT_IMMEDIATE or a positive value.
**   SA_ERR_HPI_READ_ONLY is returned if the auto-extract timeout for the
**      resource is fixed, as indicated by the 
**      SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY in the resource's RPT entry.
**   
** Remarks:
**   This function accepts a parameter instructing the resource to impose a
**   delay before performing its default hot swap policy for auto-extraction. 
**   The parameter may be set to SAHPI_TIMEOUT_IMMEDIATE to direct the resource
**   to proceed immediately to auto-extraction, or to SAHPI_TIMEOUT_BLOCK to 
**   prevent auto-extraction from ever occurring on a resource. If the parameter
**   is set to another value, then it defines the number of nanoseconds between
**   the time a hot swap event with 
**   HotSwapState = SAHPI_HS_STATE_EXTRACTION_PENDING is generated and the time
**   that the auto-extraction policy will be invoked for the resource. If, 
**   during this time period, a saHpiHotSwapPolicyCancel() function call is 
**   processed, the timer will be stopped, and the auto-extraction policy will
**   not be invoked.
** 
**   Once the auto-extraction policy begins, an HPI User will not be allowed to
**   cancel the extraction policy; hence, the timeout should be set
**   appropriately to allow for this condition. Note that the timeout period
**   begins when the hot swap event with 
**   HotSwapState = SAHPI_HS_STATE_EXTRACTION_PENDING is initially generated;
**   not when it is received by a HPI User with a saHpiEventGet() function call,
**   or even when it is placed in a session event queue. 
**
**   The auto-extraction policy is set at the resource level and is only
**   supported by resources supporting the Managed Hot Swap capability. The
**   auto-extraction timeout value cannot be modified if the resource's "Hot
**   Swap AutoExtract Read-only" capability is set. After discovering that a 
**   newly inserted resource supports Managed Hot Swap, and read-write 
**   auto-extract timeouts, an HPI User may use this function to change the 
**   timeout of the auto-extraction policy for that resource. If a resource 
**   supports the simplified hot swap model, setting this timer has no effect 
**   since the resource will transition directly to NOT PRESENT state on an 
**   extraction.
**
**   An implementation may enforce a fixed, preset timeout value.  In such 
**   cases, the SAHPI_HS_CAPABILITY_AUTOEXTRACT_READ_ONLY flag will be set to
**   indicate that an HPI User cannot change the auto-extract Timeout value.
**   SA_ERR_HPI_READ_ONLY is returned if an HPI User attempts to change a 
**   read-only timeout.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiAutoExtractTimeoutSet(
    SAHPI_IN  SaHpiSessionIdT      SessionId,
    SAHPI_IN  SaHpiResourceIdT     ResourceId,
    SAHPI_IN  SaHpiTimeoutT        Timeout
);

/*******************************************************************************
**
** Name: saHpiHotSwapStateGet()
**
** Description:
**   This function allows an HPI User to retrieve the current hot swap state of
**   a resource. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   State - [out] Pointer to location to store returned state information.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support managed
**      hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in the 
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the State pointer is passed in as
**      NULL.
**   
** Remarks:
**   The returned state will be one of the following four states:
**      * SAHPI_HS_STATE_INSERTION_PENDING
**      * SAHPI_HS_STATE_ACTIVE
**      * SAHPI_HS_STATE_EXTRACTION_PENDING
**      * SAHPI_HS_STATE_INACTIVE
**
**   The state SAHPI_HS_STATE_NOT_PRESENT will never be returned, because a
**   resource that is not present cannot be addressed by this function in the
**   first place.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiHotSwapStateGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_OUT SaHpiHsStateT         *State
);

/*******************************************************************************
**
** Name: saHpiHotSwapActionRequest()
**
** Description:
**   This function allows an HPI User to invoke an insertion or extraction 
**   process via software. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   Action - [in] Requested action:  
**      * SAHPI_HS_ACTION_INSERTION
**      * SAHPI_HS_ACTION_EXTRACTION
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support managed
**      hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in the 
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the resource is not in an 
**      appropriate hot swap state, or if the requested action is inappropriate
**      for the current hot swap state.  See the Remarks section below.
**   SA_ERR_HPI_INVALID_PARAMS is returned when Action is not one of the valid
**      enumerated values for this type.
**   
** Remarks:
**   A resource supporting hot swap typically requires a physical action on the
**   associated FRU to invoke an insertion or extraction process. An insertion
**   process is invoked by physically inserting the FRU into a chassis. 
**   Physically opening an ejector latch or pressing a button invokes the 
**   extraction process.  This function provides an alternative means to invoke
**   an insertion or extraction process via software.
**
**   This function may only be called:
**      * To request an insertion action when the resource is in INACTIVE state.
**      * To request an extraction action when the resource is in the ACTIVE 
**         state.
**   The function may not be called when the resource is in any other state.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiHotSwapActionRequest (
    SAHPI_IN SaHpiSessionIdT     SessionId,
    SAHPI_IN SaHpiResourceIdT    ResourceId,
    SAHPI_IN SaHpiHsActionT      Action
);

/*******************************************************************************
**
** Name: saHpiHotSwapIndicatorStateGet()
**
** Description:
**   This function allows an HPI User to retrieve the state of the hot swap
**   indicator. A FRU associated with a hot-swappable resource may include a hot
**   swap indicator such as a blue LED. This indicator signifies that the FRU is
**   ready for removal. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   State - [out] Pointer to location to store state of hot swap indicator.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support:
**      * Managed hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in
**         the resource's RPT entry.
**      * A hot swap indicator on the FRU as indicated by the 
**        SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED in the resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the State pointer is passed in as
**      NULL.
**   
** Remarks:
**   The returned state is either SAHPI_HS_INDICATOR_OFF or 
**   SAHPI_HS_INDICATOR_ON. This function will return the state of the 
**   indicator, regardless of what hot swap state the resource is in.
**
**   Not all resources supporting managed hot swap will necessarily support 
**   this function. Whether or not a resource supports the hot swap indicator
**   is specified in the Hot Swap Capabilities field of the RPT entry.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiHotSwapIndicatorStateGet (
    SAHPI_IN  SaHpiSessionIdT            SessionId,
    SAHPI_IN  SaHpiResourceIdT           ResourceId,
    SAHPI_OUT SaHpiHsIndicatorStateT     *State
);

/*******************************************************************************
**
** Name: saHpiHotSwapIndicatorStateSet()
**
** Description:
**   This function allows an HPI User to set the state of the hot swap 
**   indicator. A FRU associated with a hot-swappable resource may include a hot
**   swap indicator such as a blue LED. This indicator signifies that the FRU is
**   ready for removal. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   State - [in] State of hot swap indicator to be set.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support:
**      * Managed hot swap, as indicated by SAHPI_CAPABILITY_MANAGED_HOTSWAP in
**         the resource's RPT entry.
**      * A hot swap indicator on the FRU as indicated by the 
**         SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED in the resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when State is not one of the valid 
**      enumerated values for this type.
**   
** Remarks:
**   Valid states include SAHPI_HS_INDICATOR_OFF or SAHPI_HS_INDICATOR_ON. This
**   function will set the indicator regardless of what hot swap state the 
**   resource is in, though it is recommended that this function be used only
**   in conjunction with moving the resource to the appropriate hot swap state.
**
**   Not all resources supporting managed hot swap will necessarily support this
**   function. Whether or not a resource supports the hot swap indicator is 
**   specified in the Hot Swap Capabilities field of the RPT entry.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiHotSwapIndicatorStateSet (
    SAHPI_IN SaHpiSessionIdT           SessionId,
    SAHPI_IN SaHpiResourceIdT          ResourceId,
    SAHPI_IN SaHpiHsIndicatorStateT    State
);

/*******************************************************************************
**
** Name: saHpiParmControl()
**
** Description:
**   This function allows an HPI User to save and restore parameters associated
**   with a specific resource. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   Action - [in] Action to perform on resource parameters.
**      * SAHPI_DEFAULT_PARM  Restores the factory default settings for a 
**         specific resource. Factory defaults include sensor thresholds and 
**         configurations, and resource- specific configuration parameters.
**      * SAHPI_SAVE_PARM     Stores the resource configuration parameters in
**         non-volatile storage. Resource configuration parameters stored in
**         non-volatile storage will survive power cycles and resource resets.
**      * SAHPI_RESTORE_PARM  Restores resource configuration parameters from
**         non-volatile storage. Resource configuration parameters include 
**         sensor thresholds and sensor configurations, as well as 
**         resource-specific parameters.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support 
**      parameter control, as indicated by SAHPI_CAPABILITY_CONFIGURATION in the
**      resource's RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when Action is not set to a proper 
**      value.
**   
** Remarks:
**   Resource-specific parameters should be documented in an implementation 
**   guide for the HPI implementation.
**
**   When this API is called with SAHPI_RESTORE_PARM as the action prior to
**   having made a call with this API where the action parameter was set to
**   SAHPI_SAVE_PARM, the default parameters will be restored.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiParmControl (
    SAHPI_IN SaHpiSessionIdT      SessionId,
    SAHPI_IN SaHpiResourceIdT     ResourceId,
    SAHPI_IN SaHpiParmActionT     Action
);

/*******************************************************************************
**
** Name: saHpiResourceResetStateGet()
**
** Description:
**   This function gets the reset state of an entity, allowing an HPI User to 
**   determine if the entity is being held with its reset asserted. If a 
**   resource manages multiple entities, this function will address the entity
**   which is identified in the RPT entry for the resource.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   ResetAction - [out] The current reset state of the entity. Valid reset 
**      states are:
**      * SAHPI_RESET_ASSERT:     The entity's reset is asserted, e.g., for hot
**         swap insertion/extraction purposes.
**      * SAHPI_RESET_DEASSERT:   The entity's reset is not asserted.
**
** Return Value:
**   SA_OK is returned if the resource has reset control, and the reset state
**      has successfully been determined; otherwise, an error code is returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support reset
**      control as indicated by SAHPI_CAPABILITY_RESET in the resource's RPT 
**      entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the ResetAction pointer is passed
**      in as NULL.
**   
** Remarks:
**   SAHPI_COLD_RESET and SAHPI_WARM_RESET are pulsed resets, and are not valid
**   values to be returned in ResetAction. If the entity is not being held in
**   reset (using SAHPI_RESET_ASSERT), the appropriate value is 
**   SAHPI_RESET_DEASSERT. 
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceResetStateGet (
    SAHPI_IN SaHpiSessionIdT        SessionId,
    SAHPI_IN SaHpiResourceIdT       ResourceId,
    SAHPI_OUT SaHpiResetActionT     *ResetAction
);

/*******************************************************************************
**
** Name: saHpiResourceResetStateSet()
**
** Description:
**   This function directs the resource to perform the specified reset type on 
**   the entity that it manages. If a resource manages multiple entities, this
**   function addresses the entity that is identified in the RPT entry for the
**   resource. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   ResetAction - [in] Type of reset to perform on the entity. Valid reset 
**      actions are:
**      * SAHPI_COLD_RESET:     Perform a 'Cold Reset' on the entity (pulse), 
**         leaving reset de-asserted,
**      * SAHPI_WARM_RESET:     Perform a 'Warm Reset' on the entity (pulse),
**         leaving reset de-asserted,
**      * SAHPI_RESET_ASSERT:   Put the entity into reset state and hold reset
**         asserted, e.g., for hot swap insertion/extraction purposes,
**      * SAHPI_RESET_DEASSERT: Bring the entity out of the reset state.
**
** Return Value:
**   SA_OK is returned if the resource has reset control, and the requested 
**     reset action has succeeded; otherwise, an error code is returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support resource
**      reset control, as indicated by SAHPI_CAPABILITY_RESET in the resource's
**      RPT entry. 
**   SA_ERR_HPI_INVALID_PARAMS is returned when the ResetAction is not set to a
**      proper value.
**   SA_ERR_HPI_INVALID_CMD is returned if the requested ResetAction is 
**     SAHPI_RESET_ASSERT and the resource does not support this action.
**   SA_ERR_HPI_INVALID_REQUEST is returned if the ResetAction is 
**     SAHPI_COLD_RESET or SAHPI_WARM_RESET and reset is currently asserted.
**   
** Remarks:
**   Some resources may not support holding the entity in reset.  If this is the
**   case, the resource should return SA_ERR_HPI_INVALID_CMD if the 
**   SAHPI_RESET_ASSERT action is requested.  All resources that have the
**   SAHPI_CAPABILITY_RESET flag set in their RPTs should support 
**   SAHPI_COLD_RESET and SAHPI_WARM_RESET.  However, it is not required that
**   these actions be different.  That is, some resources may only have one 
**   sort of reset action (e.g., a "cold" reset) which is executed when either
**   SAHPI_COLD_RESET or SAHPI_WARM_RESET is requested.
**
**   The SAHPI_RESET_ASSERT is used to hold an entity in reset, and 
**   SAHPI_RESET_DEASSERT is used to bring the entity out of an asserted reset
**   state.  
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourceResetStateSet (
    SAHPI_IN SaHpiSessionIdT       SessionId,
    SAHPI_IN SaHpiResourceIdT      ResourceId,
    SAHPI_IN SaHpiResetActionT     ResetAction
);

/*******************************************************************************
**
** Name: saHpiResourcePowerStateGet()
**
** Description:
**   This function gets the power state of an entity, allowing an HPI User to
**   determine if the entity is currently powered on or off. If a resource 
**   manages multiple entities, this function will address the entity which is 
**   identified in the RPT entry for the resource.
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   State - [out] The current power state of the resource.  Valid power states
**      are:
**      * SAHPI_POWER_OFF: The entity's primary power is OFF,
**      * SAHPI_POWER_ON: The entity's primary power is ON.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned. 
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support power
**      management, as indicated by SAHPI_CAPABILITY_POWER in the resource's 
**      RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned if the State pointer is passed in
**      as NULL.
**   
** Remarks:
**   SAHPI_POWER_CYCLE is a pulsed power operation and is not a valid return
**   for the power state.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourcePowerStateGet (
    SAHPI_IN  SaHpiSessionIdT       SessionId,
    SAHPI_IN  SaHpiResourceIdT      ResourceId,
    SAHPI_OUT SaHpiPowerStateT      *State
);

/*******************************************************************************
**
** Name: saHpiResourcePowerStateSet()
**
** Description:
**   This function directs the resource to perform the power control action on
**   the entity that it manages. If a resource manages multiple entities, this
**   function addresses the entity that is identified in the RPT entry for the
**   resource. 
**
** Parameters:
**   SessionId - [in] Identifier for a session context previously obtained using
**      saHpiSessionOpen().
**   ResourceId - [in] Resource identified for this operation.
**   State - [in] the requested power control action.  Valid values are:
**      * SAHPI_POWER_OFF: The entity's primary power is turned OFF,
**      * SAHPI_POWER_ON: The entity's primary power is turned ON,
**      * SAHPI_POWER_CYCLE: The entity's primary power is turned OFF, then
**         turned ON.
**
** Return Value:
**   SA_OK is returned on successful completion; otherwise, an error code is
**      returned.
**   SA_ERR_HPI_CAPABILITY is returned if the resource does not support power
**      management, as indicated by SAHPI_CAPABILITY_POWER in the resource's 
**      RPT entry.
**   SA_ERR_HPI_INVALID_PARAMS is returned when State is not one of the valid 
**      enumerated values for this type.
**   
** Remarks:
**   This function controls the hardware power on a FRU entity regardless of 
**   what hot-swap state the resource is in. For example, it is legal (and may
**   be desirable) to cycle power on the FRU even while it is in ACTIVE state in
**   order to attempt to clear a fault condition. Similarly, a resource could be
**   instructed to power on a FRU even while it is in INACTIVE state, for 
**   example, in order to run off-line diagnostics.
** 
**   Not all resources supporting hot swap will necessarily support this 
**   function. In particular, resources that use the simplified hot swap model
**   may not have the ability to control FRU power. 
**
**   This function may also be supported for non-FRU entities if power control
**   is available for those entities.
**
*******************************************************************************/
SaErrorT SAHPI_API saHpiResourcePowerStateSet (
    SAHPI_IN SaHpiSessionIdT       SessionId,
    SAHPI_IN SaHpiResourceIdT      ResourceId,
    SAHPI_IN SaHpiPowerStateT      State
);

#endif
