/******************************************************************************
 * Copyright (c) 2013 Audio Analytic Ltd, Cambridge, UK                       *
 *                                                                            *
 * Public interface functions header                                          *
 ******************************************************************************/

#ifndef AUDIOANALYTIC_PUBLIC_HEADER
#define AUDIOANALYTIC_PUBLIC_HEADER

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief A quantity referring to a size
///
/// The \a aa_size_t type allows a platform independent measure of counts.
/// As size_t is likely to be 64-bits wide on a 64-bit architecture
/// this is defined to be an uint32_t to allow for the same size
/// quantity across 32 and 64-bit architectures.  Since the amount
/// of data CoreLogger is processing is comparatively small there
/// is no need for such large values for size quantities.

typedef uint32_t aa_size_t;

/// \brief Fixed point data structures and utilities
///
/// Internally CoreLogger uses fixed point calculations for doing decimal
/// calculations.  These are organised as 16 bits for the whole part
/// and 16 bits for the fractional parts - known as Q16.16.  The following
/// types and routines assist developers switching to and from standard
/// C/C++ float types to fixed point types.  Care should be taken when
/// coverting from floating poin to fixed point as the number can be too
/// large to represent in fixed point.  Additionally fixed point naturally
/// provides less precision than floating point - typically accuracy is
/// between 2 to 3 decimal places.

typedef int aa_fix16_t;



/// \brief Maximum value of a Q16.16 number
///
/// This is the maximum value of a Q16.16 number, roughly equivalent to
/// 32767.99998474121
#define AA_FIX16_MAX 0x7fffffff
/// \brief Minimum value of a Q16.16 number
///
/// This is the smallest value of a Q16.16 (as in the numerically lowest,
/// not the smallest precision) and is roughly equivalent -32768.0.
#define AA_FIX16_MIN 0x80000000
/// \brief Precision of a Q16.16 number
///
/// This value is the floating point equivalent of the smallest delta
/// such that two Q16.16 numbers are consider distinct.  This corresponds
/// to the floating point value 'epsilon'
#define AA_FIX16_PREC 1.52587890625e-05

/// \struct aa_audio_levels_t
/// \brief Structure containing audio level information
///
/// This structure holds information about the audio levels in the
/// last analysed frame of samples.  The structure contains the average
/// level, the maximum level and the frequency of the maximum audio level.

typedef struct {

    aa_fix16_t average_level_db; //!< Indicates the average loudness of the
                                 //!<  audio in the last second, as decibels
                                 //!< in fixed point 16.16 format 

    aa_fix16_t max_level_db;     //!< Indicates the loudness level of the
                                 //!< loudest audio band, as decibels in
                                 //!< fixed point 16.16 format 

    int        max_level_hz;     //!< Central frequency of band containing
                                 //!< the loudest sound, integer
} aa_audio_levels_t;

/// \brief Convenience type for a score
///
/// To help distinguish use cases of Q16.16 numbers this type represents
/// scores as opposed to other values.

typedef aa_fix16_t aa_score_t;


/// \brief CoreLogger error code values
///
/// Most CoreLogger routines return error codes to indicate whether or not
/// the process was successful.  A successful return value is AA_ERROR_OK,
/// which has the value 0.  All error codes have negative values making it
/// easy to check for failure conditions.  In some cases the calling code
/// may not be able to change much to affect the outcome (for example in the
/// case of AA_ERROR_MEMORY when the system is low on heap).  In other cases
/// there may be a useful action the code can undertake on receiving the 
/// error.
typedef enum {
    AA_ERROR_OK             =  0,  /*!< Success, no error occurred */
    AA_ERROR_ERROR          = -1,  /*!< An unspecified error occurred */
    AA_ERROR_INVALID_ID     = -2,  /*!< The given CoreLogger id was not valid */
    AA_ERROR_SOUNDPACK      = -3,  /*!< The Sound Pack could not be loaded 
                                      because of a format error */
    AA_ERROR_MEMORY         = -4,  /*!< An error has occurred allocating 
                                     memory */
    AA_ERROR_INPUT          = -5,  /*!< Error in the input to an audioanalytic
                                     function */
    AA_ERROR_RESULTS        = -6,  /*!< Error in supplied results structure */
    AA_ERROR_LOG_CANT_OPEN  = -7,  /*!< Can't open the log file */
    AA_ERROR_LOG_ALREADY_ENABLED = -8,  /*!< Logging is already enabled */
    AA_ERROR_LOG_FCLOSE     = -9,  /*!< An error occurred when closing the log
                                     file */
    AA_ERROR_COMPATIBILITY  = -10, /*!< The Sound Pack could not be loaded
                                     because it is incompatible with this
                                     build of Corelogger */
    AA_ERROR_SAMPLERATE     = -11, /*!< The Sound Pack could not be loaded
                                     because its sample rate is incompatible
                                     with this instance of Corelogger */
    AA_ERROR_SOUNDPACK_MISSING = -12, /*!< The Sound Pack cound not be loaded 
                                        because it could not be found */
    AA_ERROR_SOUNDPACK_NOT_FOUND = -13, /*!< The sound pack id cannot be
                                          located */
    AA_ERROR_INCOMPLETE_SENSITIVITY = -14, /*!< The sensitivity settings in the
                                             JSON config file are incomplete */
    AA_ERROR_INVALID_JSON = -15,   /*!< JSON parsing encountered a problem */
    AA_ERROR_MISSING_FILE = -16,   /*!< JSON definition of a soundpack is
                                     missing the file */
    AA_ERROR_THRESHOLD_TYPE = -17, /*!< JSON threshold definition is not a
                                     numeric type */
    
} AA_ERROR_T;



/// \brief Type for sound pack or model identifiers
///
/// A variety of CoreLogger functions work on models and sound packs.
/// (Conceptually a SoundPack is a collection of related models and, as
/// a consequence, tend to be easier to use.)  A model or sound pack,
/// when loaded, will be assigned an identifier which the calling code
/// can use to reference the given sound pack or model.  Such operations
/// including starting and stopping, checking to see if it is currently
/// active or iterating through all loaded sound packs or models.

typedef uint32_t aa_spid_t;


/// \brief A value indicating that the sound pack or model identifier is not
/// valid
///
/// In a number of cases the library returns values to indicate that sound
/// pack or model identifier is not valid - this value is used to indicate
/// that case.
#define AA_SPID_UNKNOWN 0xffffffff



/// \brief Opaque type representing a CoreLogger instance
///
/// Almost all CoreLogger routines operate on a given CoreLogger instance.
/// This type is the type that holds the handle to the given CoreLogger
/// instance.  The calling code should make no assumptions about this
/// handle and should assume that it is allocated on the heap.  On some
/// platforms (especially small embedded devices) the instance could be
/// statically allocated and calling free on this will cause a crash or
/// abort.
typedef void* cl_instance_t;


/// \brief Type of event
///
/// When using the events API call to get detected events in the last frame
/// detected events will have an aa_evtype_t to indicate what the event type
/// detected was.  Callers should be aware that due to the way the detection
/// works it is possible to see AA_BEGIN_EVT and AA_END_EVT events occurring
/// multiple times even when the sound is continuous.  Calling code should
/// anticipate this and potentially set a timer to only clear an event after
/// a reasonable elapsed time - or not report an event that has not had
/// sufficient duration to be considered stable.
typedef enum {
    AA_BEGIN_EVT, /*!< Event is a detection of a new event */
    AA_END_EVT,   /*!< Event is no longer being detected */
} aa_evtype_t;


/// \brief Structure for reporting events
///
/// For each event detected regardless of whether the model was part of a
/// a sound pack or an free standing model an event will be returned.  When
/// the model is part of a sound pack then it will be referenced in this
/// structure as well as reporting the actual model that detected a sound.
/// The structure gives some ancilliary information such as the time, in 
/// milliseconds, in the audio stream that the event occurred.  Note that
/// the time in the audio stream is reported in terms of the frames
/// delivered to CoreLogger and as such it represents the real time in the
/// audio stream which may, or may not, relate to wall time.  If the caller
/// needs to link this to wall time then it is responsible for keeping
/// track of that time.
/// Additionally the event contains the scores that were observed to allow
/// The user to determine if the match was a very good match (i.e. the
/// score value should be noticeably lower than than the threshold value)
/// or only a poor match, the difference between the threshold and the score
/// is small.  The last_score field allows users to determine the difference
/// between the two values and how the scores have been changing.
typedef struct {
    aa_evtype_t evtype;   /*!< The type of this event */
    long long ms_time;    /*!< The number of milliseconds into
                            the sample when this detection occurred */
    char* model_name;     /*!< The name of the model that detected */
    aa_spid_t mid;        /*!< The detecting model's identifier */
    char* sp_name;        /*!< If the model is associated with a sound pack
                            this contains a read-only copy of the sound pack's
                            name */
    aa_spid_t spid;       /*!< If the model is associated with a sound pack
                            this contains the sound packs identifier */
    aa_score_t last_score;/*!< The score from the last frame of audio
                            processed */
    aa_score_t score;     /*!< The score from this frame of audio processed */
    aa_score_t threshold; /*!< The threshold setting for this model */
} aa_event_t;


/// \def AA_MAX_CURVE_POINTS
/// \brief Maximum number of points on a threshold curve
///
/// For detection the threshold values lie along a curve.  Because curves
/// are expensive to compute the sound model generation creates an
/// approximation of a series of line segments that are close to the original
/// curve.  In order to contain the complexity further, the number of points
/// on this line segment approximation is limited to this number as a maximum.
/// However, in practice many threshold curves do, in fact, contain
/// significantly less points.
#define AA_MAX_CURVE_POINTS 16



/// \brief Structure containing information about a model
///
/// Sound models define a number of parameters that are visible to the
/// caller such as the threshold, the buffer length and the sample rate.
/// Most CoreLogger sound models use 16kHz as the sample rate and so this
/// is the default.  However, occasionally there are reasons for a model
/// to operate at a different sample rate and this structure allows the
/// caller to detect such things.  Note that a CoreLogger instance can
/// only operate in one sample rate and will only load models which
/// match the CoreLogger's sample rate.
///
/// The \a aa_model_info_t structure provides the theshold curve.
/// The threshold curve is used to \a tune the sensitivity of the
/// CoreLogger analytics engine for this model.
///
/// The threshold curve is a series of line segments between two 
/// adjacent points ranged over an x range of 0 to 255.
/// Each segment will fall between two points along the 
/// \a linear_scale.  For each x value on the linear scale
/// a corresponding entry exists in the score_scale.  This value is a
/// Q16.16 value.
///
/// The user is responsible for performing a linear interpolation of the
/// intermediate values on the x axis to scale the corresponding
/// differences on the y axis for the scores.
///
/// Taking an arbitrary point along this 0-255 linear scale the calculation
/// is performed by determining first which two line segments the desired
/// x position lies and then computing the difference between the two adjacent
/// score values for the corresponding two linear scale values.  Then
/// the difference between the two corresponding linear scale values is
/// calculated.  The point increment for the score on each successive
/// x value for this segment is computed by dividing the score value
/// difference by the linear segment difference - this is the step score
/// change.  The corresponding score for the desired x value is determined
/// by subtracting the lower of the two x values of the segment from the
/// required x value and then multiplying that difference by the step
/// score change.
///
/// The complexity in performing this calculation comes primarily from
/// two sources.  The first is the fact that that segments are of
/// indeterminate size before analysis and the model determines how many
/// of there are.  The second is that the the scores are in Q16.16 format
/// and while the calculations are straightforward integer operations (one of
/// very nice properties of fixed point calculations) overflow is easy
/// to encounter and sometimes difficult to guard against.
///
/// For this reason it is recommended that unless there are good reasons
/// for using the scores directly the event interface provided by
/// @ref aa_get_frame_events is much easier to use and avoids much of these
/// complexities, largely because all the code for this linear interpolation
/// is inside the library itself.
typedef struct {
    int    sample_rate;    /*!< Sample rate (Hz) of the Sound Pack */
    int    threshold;      /*!< Recommended detection threshold, 
                             fixed point 16.16 format */
    int    buffer_length;  /*!< Length (samples) of buffer used to 
                             detect sounds */
    int    name_length;    /*!< Length of the name buffer */
    char  *name;           /*!< Pointer to buffer containing the model's name */
    int    num_points;     /*!< Number of points defined on the 
                             following curve */
    aa_score_t score_scale[AA_MAX_CURVE_POINTS]; /*!< Specifies the model output
                                               value corresponding to the
                                               linear scale at the same index,
                                               fixed point 16.16 format */
    unsigned char linear_scale[AA_MAX_CURVE_POINTS]; /*!< Specifies a linear
                                                       scale as a byte, values
                                                       0-255 represent a linear
                                                       scale of 0.0 to 1.0 */
} aa_model_info_t;


/// \brief Structure containing information about a sound pack
///
/// This structure contains information about the given sound pack such
/// as sample rate, buffer length, sound pack name, tag and other levels.
/// The name of the sound pack is fixed while the user can set the tag
/// to whatever value they choose.  The trigger level is used to determine
/// the minimum loudness level that the analytics will run at.  This
/// defaults to -90dB.  The sensitivity setting is the default sensitivity
/// for this sound pack - this is usually the half way value on the
/// sensitivity scale.  This structure also indicates the number of
/// distinct models that are contained in the sound pack.
typedef struct {
    int    sample_rate;      /*!< Sample rate (Hz) of the Sound Pack */
    int    buffer_length;    /*!< buffer length for the sound pack */
    char  *name;             /*!< name of the sound pack */
    char  *tag;              /*!< Pointer to buffer containing the 
                               Sound Pack's name */
    float  trigger_level_db; /*!< initial trigger level.  For a non loaded
                               sound pack this value is set to INFINITY */
    float  sensitivity;      /*!< default sensitivity level.  For a not loaded
                               sound pack this value is set to INFINITY */
    size_t num_models;       /*!< number of models associate with the
                               soundpack */
    aa_size_t memory;        /*!< an approximate amount of heap this
                               sound pack consumes when loaded.  This value
                               is reported as 0 for an already loaded sound
                               pack.  The intent is to allow low memory
                               devices make decisions about whether they can
                               load a sound pack by examining this value
                               before loading the sound pack */
} aa_soundpack_info_t;


/// \brief Conversion from Q16.16 to float
/// \param fx the fixed point value to convert to a float
/// \return a float value representing the given fixed point value
///
/// This helper routine converts from a Q16.16 to the nearest float
/// value.  The routine is marked as inline to allow the code to
/// be injected into users code since the calculation is very simple.
static inline float fix16_to_float( const aa_fix16_t fx )
{
    return ((float)fx)/65536.0f;
}

/// \brief Conversion from float to Q16.16
/// \param fl the floating point value to convert to fixed point
/// \return a fixed point value representing the corresponing float
///
/// This helper function converts from a floating point value to the
/// nearest Q16.16 value.  Note that if the value is greater than the
/// maximum or minimum value that can be represented in Q16.16 the 
/// appropriate maximum or minimum Q16.16 value is returned.  Calling
/// code should check for this case to ensure that the value is
/// correctly interpreted.
static inline aa_fix16_t float_to_fix16( const float fl )
{
    if (fl < -32678.0f)
    {
	return AA_FIX16_MIN;
    }
    else if (fl > 32767.0f)
    {
	return AA_FIX16_MAX;
    }
    return (aa_fix16_t)(int)(fl*65536.0f);
}



/// \brief Create a new CoreLogger instance
/// \param config_file_name is the path to a JSON configuration file
/// \param cl_instance is a pointer to a cl_instance_t object
/// \return Error code reflecting the success of the operation
///
/// The aa_new_instance function creates a new instance of the CoreLogger
/// analytics engine.  The \a config_file_name parameter contains the path 
/// to a JSON format configuration file.  The contents of this file are
/// documented in the API guide.  The configuration file will be read and
/// whatever sound packs are referenced in it will be loaded and started.
/// Additionally any sensitivity settings changes and logging settings will
/// also be acted on.
///
/// JSON typos can be hard to track down and while \a aa_new_instance does
/// attempt to help report problems it does not report where the problem
/// is.  For this reason it may be useful to run the JSON file through
/// something like a Python script to confirm that the JSON is well
/// formatted before pushing the changes live.
/// Note that on some embedded platforms where heap space is at at premium
/// CoreLogger may have been compiled to use a statically allocated
/// CoreLogger instance.  The library tracks this and will prevent the
/// user from ever instantiating more than one CoreLogger instance.  However,
/// this operation is not thread safe and calling code should
/// take steps to avoid multiple threads trying to create CoreLogger instances
/// concurrently.
///
/// \warning In multithreaded environments the caller must ensure that more
/// than one thread will not be active in \a aa_new_instance concurrently.
AA_ERROR_T aa_new_instance( const char *config_file_name, cl_instance_t *cl_instance );


/// \brief Create a new CoreLogger instance without JSON
/// \param sample_rate frequency of input samples
/// \param frame_len is the length of each block of samples
/// \param id returns the new CoreLogger instance
/// \return error code reflecting the success of the operation
///
/// The \a aa_new_instance_input_config routine allows the caller to create
/// a new CoreLogger instance without supplying a JSON file.  Using this
/// routine means that the user will have to use \a aa_add_and_start_soundpack
/// to load the sound packs thus making it more involved to change the
/// soundpacks of a running system and also requiring extra code.
///
/// Many of the same caveats and conditions apply to 
/// \a aa_new_instance_input_config just as they do to \a aa_new_instance.
///
/// \warning This routine is intended to be used in embedded enironments and
/// should not be used in environments where a file system is available as
/// the \a aa_new_instance routine is much more flexible and the use of JSON
/// simplifies configuration management across devices.
AA_ERROR_T aa_new_instance_input_config( const int sample_rate, const int frame_len, cl_instance_t* id);


/// \brief Perform analysis on sound frame
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \param frame a buffer of \a frame_len samples
/// \param frame_length number of samples int the \a frame buffer
/// \return an error code indicating the success of the operation
///
/// The \a aa_classify_sound_frame call performs the analysis of the audio
/// and collects the scores for matches.  This call is processor intensive
/// albeit that it uses fixed point internally which is usually faster
/// than floating point.  Care should be taken to ensure that the processor
/// has sufficient capacity for the processing performed by this routine.
///
/// If running in a real-time analysis environment the system should be checked
/// to ensure that the delivery of audio to this routine and its subsequent
/// processing happen with a valid time frame.
AA_ERROR_T aa_classify_sound_frame( const cl_instance_t cl_instance, const short *frame, const int frame_length );


/// \brief Retrieve audio levels for last processed frame
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param levels a pointer to a caller allocated @ref aa_audio_levels_t struct
/// \return an error code indicating the success of the operation
///
/// The \a aa_get_audio_levels routine allows the caller to extract some
/// useful information regarding the audio.  Specifically it allows the caller
/// to retrieve the maximum audio level for the last processed frame, the
/// average audio level for the last processed frame and the frequence of the
/// maximum audio level for the last processed frame.  These values are
/// calculated in the process of performing the analysis so this call is
/// very inexpensive.
///
/// For sound detection purposes it is important that the audio level is
/// sufficiently loud to be able to detect useful information.  The default
/// level is set to -90dB and can be changed by @ref aa_set_trigger_level
/// or @ref aa_set_trigger_level_fx.  If the level is adjusted then it
/// may be useful to call this function to track what levels the audio is
/// arriving at.
AA_ERROR_T aa_get_audio_levels( const cl_instance_t cl_instance, aa_audio_levels_t *levels );


/// \brief Retrieve the scores for the last frame
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param mid a model identifier to get the score for
/// \param score a pointer to receive the score value
/// \return an error code indicating the success of the operation
///
/// The \a aa_get_frame_score allows the used to retrieve the score for the
/// last frame of audio processed for the given model.  Since the CoreLogger
/// analytics engine works at the level of models here is a place where the
/// model identifier must be used.
///
/// For each active model the computed match will be returned in \a score.
/// Since each score is returned one at a time there is no need (if not
/// required by the calling code) to create an array to hold the scores
/// for all loaded and active models.
///
/// For detection the score must fall below the threshold value retrieved
/// via a call to @ref aa_get_model_info or @ref aa_get_model_info_from_mid
/// Adjustment along the threshold curve must also be managed if using this
/// interface.  See @ref aa_model_info_t for more information on computing
/// threshold curves.
///
/// If the requested model is stopped (@ref aa_is_model_active returns 0)
/// then the score will be returned as 0.
///
/// For easier iteration through all available models see the 
/// @ref aa_get_first_model, @ref aa_end_model and @ref aa_get_next_model
/// calls.
AA_ERROR_T aa_get_frame_score( const cl_instance_t cl_instance, const aa_spid_t mid, aa_score_t *score );


/// \brief Retrieve detected events for the last frame
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param events the address of a pointer to receive an array of 
/// @ref aa_event_t structures
/// \param nbr_events a pointer to an integer
/// \return an error code indicating the success of the operation
///
/// The \a aa_get_frame_events routine allows the the caller to retrieve
/// the events that occured during the last frame.  These are essentially
/// the detections or loss of detection of individual sounds.  The \a events
/// parameter receives an array, allocated by the library, of events
/// identified in the last processed frame.  The \a nbr_events parameter
/// is a pointer to a caller allocated integer to receive the count of events.
///
/// The caller should assume that this routine will often return no events
/// and in this case the pointer who address was provided in \a events will be
/// set to \a NULL.  Additionally the integer pointer to by the pointer 
/// \a nbr_events will be set to 0.
///
/// Once the user has processed the events the calling code should call
/// the platform's free() routine on pointer to release the memory back to
/// the heap.
AA_ERROR_T aa_get_frame_events( const cl_instance_t cl_instance, aa_event_t** events, int *nbr_events );


/// \brief Destroy a valid CoreLogger instance
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return an error code indicating the success of the operation
///
/// The \a aa_delete_instance routine destroys a CoreLogger instance.
/// All models, sound packs and any other data structures that that
/// CoreLogger has created will be freed.  Additionally some pointers
/// to items such as sound pack names and model names will also be
/// destroyed in the process.
///
/// A consequence of calling \a aa_delete_instance is clearly that the
/// \a cl_instance value may no longer be used.  Also any of the other
/// names values should also be considered to have been removed and
/// released back to the system.
///
/// However, the structures that are indicated that it is the caller's
/// responsibility to free will remain valid after the destruction
/// of a CoreLogger instance.
AA_ERROR_T aa_delete_instance( const cl_instance_t cl_instance );


/// \brief Retrieve the number of sound packs loaded
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \return the number of sound packs loaded
///
/// The \a aa_get_number_of_soundpacks retrieves the count of loaded
/// sound packs.  If the \a cl_instance parameter is invalid this
/// routine will return 0.
aa_size_t aa_get_number_of_soundpacks( cl_instance_t cl_instance );


/// \brief Retrieve an array of all loading sound pack identifiers
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid_array the address of a pointer to receieve the allocated
/// array of sound pack identifiers
/// \param nbr_spids a pointer to a caller allocated integer to count
/// the number of sound pack identifiers
/// \return error code indicating the success of the operation
///
/// The \a aa_get_all_spids routine returns an array of sound pack identifiers
/// in \a spid_array.  The count of identifiers is returned in the \a nbr_spids
/// parameter.
///
/// When there are no sound packs loaded the integer pointer to by the
/// \a nbr_spids pointer will be set to 0 and the pointer whose address was
/// passed in through \a spid_array will be set to NULL.
///
/// When there is one or more sound packs loaded it is the caller's 
/// responsibility to free the pointer array returned through
/// \a spid_array
AA_ERROR_T aa_get_all_spids( cl_instance_t cl_instance, aa_spid_t **spid_array, aa_size_t *nbr_spids );


/// \brief Retrieve the first sound pack identifier
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \return a sound pack identifier
///
/// The \a aa_get_first_spid routine returns the first found sound pack
/// identifier.  First in this instance means the order in which they
/// are stored internally with no guarantee of any particular order.
///
/// The \a aa_get_first_spid routine essentially forms part of a C++
/// like iterator with @ref aa_end_spid and @ref aa_get_next_spid.
/// The iterator is intended to be used like this:
///
/// \code
///     for (spid = aa_get_first_spid(cl_instance);
///          aa_end_spid(cl_instance);
///          spid = aa_get_next_spid(cl_instance))
///     {
///         .... operations on spid ....
///     }
/// \endcode
///
/// This loop will iterate through the loaded sound packs with
/// \a spid containing the sound pack identifier of each sound pack
/// in turn.
///
/// \warning Because the iterator state is stored in the CoreLogger
/// instance it is not possible to have two seperate iterators in 
/// operation concurrently within the same CoreLogger instance.  It is
/// possible to have a sound pack iterator and a model iterator (see
/// @ref aa_get_first_model, @ref aa_end_model and @ref aa_get_next_model)
/// concurrently and also iterators operator accross different
/// CoreLogger instances also do not interact.
aa_spid_t aa_get_first_spid( cl_instance_t cl_instance );


/// \brief Determine end of sound pack looping iterator
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return C type boolean value (0 for false, not 0 for true)
///
/// The \a aa_end_spid routine returns whether the end of the
/// iteration process has been reached.
int aa_end_spid( cl_instance_t cl_instance );


/// \brief Retrieve the next sound pack identifier for this iteration
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return a valid sound pack identifier
///
/// The \a aa_get_next_spid routine returns the next iterator in the 
/// sequence following an initialisation with @ref aa_get_first_spid.
///
aa_spid_t aa_get_next_spid( cl_instance_t cl_instance );


/// \brief Load a model from a file
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param name path to a .bsm file
/// \param ppindex the address of a pointer to an integer to receive the
/// model identifier in.
///
/// The \a aa_load_model routine allows the caller to load the .bsm files
/// directly into a CoreLogger instance.  The \a name parameter should be
/// understood by the \a aa_file_open function.  For a traditional file
/// system this will be a path.  On embedded devices it may simply be a
/// name to identifier a block of data that is returned through the aa_file*
/// family of functions.
///
/// The \a ppindex pointer will cause the integer that the pointer to pointer
/// points to will be set with the model identifier.
AA_ERROR_T aa_load_model(cl_instance_t cl_instance, const char *name, int **ppindex);



/// \brief Get the first model from a model iterator
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return a model identifier
///
/// The \a aa_get_first_model routine is intended to be used as part of an
/// iterator along with @ref aa_end_model and @ref aa_get_next_model.  The
/// outline of how to use it is as follows:
///
/// \code
///     for (mid = aa_get_first_model(cl_instance);
///          aa_end_model(cl_instance);
///          mid = aa_get_next_mid(cl_instance))
///     {
///         .... operations on spid ....
///     }
/// \endcode
///
aa_spid_t aa_get_first_model( cl_instance_t cl_instance );


/// \brief Test to see if the end of the iterator has been reached
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return C type boolean value (0 for false, not 0 for true)
///
/// The \a aa_end_model routine provides a test to identify whether
/// the end of the iterator has yet been reached.
///
/// To be used in conjunction with @ref aa_get_first_model and 
///@ref aa_get_next_model
int aa_end_model( cl_instance_t cl_instance );


/// \brief Retreive the next model identifier for the iterator
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \return a valid model identifier
///
/// The \a aa_get_next_model routine will emit the next previously 
/// non-returned model identifier.
///
/// This routine is intended to be used with @ref aa_get_first_model and
/// @ref aa_end_model.
aa_spid_t aa_get_next_model( cl_instance_t cl_instance );


/// \brief Retrieve information about the specified model
/// \param filename the name of a file containing a model
/// \param info a pointer info a user allocated structure to receive the
/// information about the model
/// \return an error code indicating the success of the operation
///
/// The \a aa_get_model_info routine retrieves the data about the model.
/// Note taht the \a info parameter is caller allocated and contains
/// some array structures which need to be created in user code.
///
/// The contents of \a info allow the user to set up threshold curves
/// and also discover the models name.
///
/// The \a aa_get_model_info is unusual in that it does not require
/// a CoreLogger instance to operate.
AA_ERROR_T aa_get_model_info(const char *filename, aa_model_info_t *info);


/// \brief Retrieve model information a specific model based on an model
/// identifier
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \param mid a model identifier to find the information for
/// \param info a pointer to a user allocated @ref aa_model_info_t structure.
/// \return an error code indicating the success of the operation.
///
/// The \a aa_get_model_info_from_mid returns the same information as
/// @ref aa_get_model_info but allows the information to be retrieved
/// for an already loaded model without have to go to disk.
///
/// The \a mid parameter indicates which model information is being sought 
/// for.  The \a info pointer is analagous to the similar parameter in
/// @ref aa_get_model_info.
AA_ERROR_T aa_get_model_info_from_mid(cl_instance_t cl_instance, aa_spid_t mid, aa_model_info_t *info);



/// \brief Start analytics for a specific model
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param mid is the model identifier to start processing
/// \return an error indicating the success of the operation
/// 
/// The \a aa_start_model routine enables a model to be processed.  By
/// default all models are loaded as `started`.  For this reason 
/// \a aa_start_model is only likely to be necessary if @ref aa_stop_soundpack
/// or @ref aa_stop_model have been called (and in the case of 
/// @ref aa_stop_soundpack the model specified in \a mid is in the
/// sound pack referenced in the previous call to @ref aa_stop_soundpack
AA_ERROR_T aa_start_model(const cl_instance_t cl_instance, const aa_spid_t mid);



/// \brief Stop analytics for a specific model
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param mid a model to stop the analytics processing on
/// \return an error code indicating the success of the operation
///
/// The \a aa_stop_model routine sets a model as not-active.  This means that
/// the engine, when passed a frame of audio in @ref aa_classify_sound_frame,
/// will not compute any values for this sound pack.  No events will be reported
/// from this model and the model's scores will be set to 0.
///
/// Use @ref aa_start_model to restart the model and begin received the scores
/// from now on.
AA_ERROR_T aa_stop_model(const cl_instance_t cl_instance, const aa_spid_t mid);


/// \brief Determine is a specific model is active or not
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param mid the identifier of the model to discover the state of
/// \return C type boolean value (0 for false, not 0 for true)
///
/// The \a aa_is_model_active routine returns either non-zero if the model,
/// is active or 0 otherwise.
int aa_is_model_active(const cl_instance_t cl_instance, const aa_spid_t mid);


/// \brief Stop a sound pack and all its associated models
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid is a sound pack identifier
/// \return an error code indicating the success of the operation
///
/// The \a aa_stop_soundpack routine will stop the models contained in the
/// sound pack.  When the sound pack is stopped all its models will return
/// 0 as their scores.
///
/// Stopping an already stopped sound pack (or contained model) will
/// succeed.
AA_ERROR_T aa_stop_soundpack( cl_instance_t cl_instance, const aa_spid_t spid );


/// \brief Start a sound pack and all its associated models
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid is a sound pack identifier
/// \return an error code indicating the success of the operation
///
/// The \a aa_start_soundpack routine will start all the models associated
/// with this sound pack.  Starting an already sound pack (or contained
/// model) will succeed.
///
/// Once the sound pack is started it will immediately begin returning
/// results.  However, the scores should not be trusted until the
/// buffer has filled (the buffer_length field of @ref aa_model_info_t)
AA_ERROR_T aa_start_soundpack( cl_instance_t cl_instance, const aa_spid_t spid );


/// \brief Check to see if a sound pack is active
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid is a sound pack identifier
/// \return C type boolean value (0 for false, not 0 for true)
///
/// The \a aa_is_active_soundpack routine checks the models in the sound pack
/// and returns true (non-zero) if *any* models in the sound pack are
/// active (i.e. started).
///
/// If *all* the sound packs associated models are stopped then thie will
/// return false (0).
AA_ERROR_T aa_is_active_soundpack( const cl_instance_t cl_instance, const aa_spid_t spid);


/// \brief Load and start a sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param file_name is the name of a the require sound pack
/// \param tag is a user meaningful name for the sound pack
/// \param spid a pointer to a aa_spid_t to receive the sound pack identifier
/// after the sound pack is loaded
/// \return an error code indicating the success of the operation
///
/// The \a aa_add_and_start_soundpack locates the sound pack data based on 
/// the file_name parameter.  Normally, on systems with a file system this
/// will be a path to the file.  However, on embedded devices implementing the
/// aa_file API it may be a name to refer to a statically linked block of
/// memory.
///
/// The tag parameter allows the user to give a more meaningful name or text
/// to the sound pack.  The routines @ref aa_get_spid, @ref aa_set_tag,
/// @ref aa_get_tag allow the user to interact with the tags and cross
/// match tags to sound packs.
///
/// The spid parameter is supplied a pointer to a caller allocated 
/// @ref aa_spid_t to receive the sound pack identifier.  This
/// identifier allows the user to interact with the sound pack.
AA_ERROR_T aa_add_and_start_soundpack( cl_instance_t cl_instance, const char *file_name, const char *tag, aa_spid_t *spid );


/// \brief Unload a sound pack and release all resources associated with it
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the identifier of the sound pack to unload
/// \return an error code indicating the success of the operation
///
/// The \a aa_unload_soundpack allows the user to unload a sound pack
/// and consequently release all associated resources.  The sound pack
/// identifier and any associated model identifiers will then become
/// unknown to the system and attempts to use them will be unsuccessful.
/// 
/// The user should ensure that any pointers that refer to the name
/// or tag of the sound pack provided by, say, @ref aa_get_soundpack_info
/// are not dereferenced as the results will be unpredictable and the
/// software may crash.
AA_ERROR_T aa_unload_soundpack( cl_instance_t cl_instance, const aa_spid_t spid);


/// \brief Retrieve information about a loaded sound pack
/// \param cl_instance is a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \param spid a sound pack identifier
/// \param info a pointer to a caller allocated @ref aa_soundpack_info_t 
/// structure to be completed by CoreLogger
///
/// The \a aa_get_soundpack_info routine allows the user to retrieve information
/// about a loaded sound pack.  This is similar to the 
/// @ref aa_get_model_info_from_mid although the information returned is
/// related to the sound pack.
///
/// The name and tag values are new allocations and it is the callers
/// responsibility to free these after use.
AA_ERROR_T aa_get_soundpack_info_from_spid( const cl_instance_t cl_instance, const aa_spid_t spid, aa_soundpack_info_t* info );


/// \brief Retrieve information from a sound pack from file
/// \param filename is the name of the sound pack file
/// \param info a pointer to a caller allocated @ref aa_soundpack_info_t 
/// structure to be completed by CoreLogger
///
/// The \a aa_get_soundpack_info routine allows the user to retrieve information
/// about a loaded sound pack.  This is similar to the 
/// @ref aa_get_model_info_from_mid although the information returned is
/// related to the sound pack.
///
/// The name and tag values are new allocations and it is the callers
/// responsibility to free these after use.
AA_ERROR_T aa_get_soundpack_info_from_file( const char* filename, aa_soundpack_info_t* info );


/// \brief Find a sound pack identifier for a given tag
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param tag the tag to be located
/// \return the matching sound pack identifier
///
/// The \a aa_get_spid routine allows the user to determine the sound pack
/// identifier related to the given \a tag value.  The sound pack identifier
/// returned will be the first found that matches the given tag.  Since the
/// CoreLogger library does not enforce uniqueness amongst tags the return
/// value is implementation dependent if duplicate tags are used across
/// different sound packs.  It is expected that the caller will ensure
/// uniqueness if this is required.
///
/// If the tag is not found the library will return the sound pack identifier
/// @ref AA_SPID_UNKNOWN
///
/// \warning This routine will do a linear search of the loaded sound packs
/// and consequently it is performance is proportional to the number of 
/// loaded sound packs.  Typically this is low and the cost is insignificant,
/// however, if a large number of sound packs are loaded the the performance
/// implications of calling this API shoudl be considered.
aa_spid_t aa_get_spid( const cl_instance_t cl_instance, const char *tag );


/// \brief Set a tag for a given sound pack identifier
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \param spid is the sound pack which is to have its tag set
/// \param tag is the tag to ascribe to this sound pack
/// \return an error code indicating the success of this operation
///
/// The \a aa_set_tag routine allows the user to set or change a tag on a
/// sound pack.  Note that while @ref aa_add_and_start_soundpack allows
/// the user to set a tag to NULL, this routine explicitly prevents that.
///
/// The library will duplicate the tag so that the caller need not keep the
/// memory allocated once the tag is set.
///
/// Any previously set tag will be freed by the library.
AA_ERROR_T aa_set_tag( cl_instance_t cl_instance, const aa_spid_t spid, const char *tag );



/// \brief Get the tag for a given sound pack identifier
/// \param cl_instance is a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid is a sound pack identifier
/// \return the found tag or NULL if the sound pack identifier is not found
///
/// The \a aa_get_tag routine allows the caller to find the tag for a given
/// sound pack identifier.
///
/// If the sound pack identifier is not found the return value will be NULL.
const char* aa_get_tag( const cl_instance_t cl_instance, const aa_spid_t spid );



/// \brief Set the sensitivity range for the CoreLogger instance
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param min the minimum value of the range
/// \param max the maximum valud of the range
/// \param nbr_points the number of points with the min and max values
///
/// The \a aa_define_sensitivity provides a mechanism to allow the caller
/// to scale the threshold values into a range that is useful to the
/// application.
///
/// Normally threshold curves lie along a series of points from 0 to 255,
/// however, in some applications (e.g. GUI or web based applications) it
/// may be more convenient to operate on a scale of say 0.0 to 1.0, or
/// 1 to 100.  Using this routine it is possible to set the range of
/// values for setting thresholds on each sound pack to a range that
/// suits better the application.
///
/// Internally the library performs the scaling required to determine
/// the point on the threshold curve so this avoids the need of the
/// caller to understand and process the threshold curve data at all.
/// Additionally this API allows the user to work with float values
/// rather than fixed point values with also makes the caller's life
/// easier.
///
/// By default the library arranges for the sensitivity scale to be
/// be 0 and 100 with 100 points.
///
/// \warning The library will reject values that provide too little
/// precision for the library to distinguish.  So for example if the 
/// caller sets the values to be min=0.0, max=1.0 with the number of
/// points being 1000000 then the precision is too small for the
/// internal fixed point representation to distinguish.  Equally if the
/// range is outside the values that may be stored in a Q16.16
/// value then these will also be rejected.  In practice more than
/// 256 individual points make little sense as the result is that
/// more than one point on the sensitivity scale will map to the same
/// point on the threshold curve leading to the ability to alter the
/// sensitivity setting without making any change to the threshold
/// value.
///
/// See @ref aa_model_info_t regarding a description of threshold curves.
AA_ERROR_T aa_define_sensitivity( cl_instance_t cl_instance, const float min, const float max, const aa_size_t nbr_points );



/// \brief Set the sensitivity value for a sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the sound pack to alter the sensitivity for
/// \param sensitivity the new value for the sennsitivity
/// \return an error code indicating the success of the opertion
///
/// The \a aa_set_sensitivity_for_soundpack routine allows the caller
/// to set the sensitivity on the pre-defined sensitivity range for
/// the given sound pack.
///
/// By default the initial sensitivity setting is the mid point on
/// the threshold curve and so corresponds to the mid point on
/// sensitivity scale.
///
/// The sensitivity value must lie within the min and max values
/// as set by @ref aa_define_sensitivity.
AA_ERROR_T aa_set_sensitivity_for_soundpack( cl_instance_t cl_instance, const aa_spid_t spid, const float sensitivity );


/// \brief Get the default sensitivity setting for a sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid is a sound pack identifier
/// \return the default sensitivity for this sound pack
///
/// The \a aa_get_default_sensitivity_for_soundpack routine allows the user
/// to retrieve the default sensitivity setting (on the user defined sensitivity
/// scale) for the given sound pack.
///
/// If the sound pack identifier does not refer to a valid, loaded, sound pack
/// then the float value INFINITY is returned.
float aa_get_default_sensitivity_for_soundpack( const cl_instance_t cl_instance, const aa_spid_t spid);


/// \brief Get the current sensitivity setting for a sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid is the sound pack identifier
/// \return the current sensitivity for this sound pack
///
/// The \a aa_get_current_sensitivity_for_soundpack routine allows the caller
/// to retrieve the current sensitivity setting (on the user defined
/// sensitivity scale) for the given sound pack.
///
/// If the sound pack identifier does not refer to a valid, loaded, sound pack
/// then the float value INFINITY is returned.
float aa_get_current_sensitivity_for_soundpack( const cl_instance_t cl_instance, const aa_spid_t spid);



/// \brief Define the sensitivity scale using Q16.16 values
/// \param cl_instance a valid CoreLogger instance created with 
/// @ref aa_new_instance
/// \param min the minimum value in the sensitivity range
/// \param max the maximum value in the sensitivity range
/// \param nbr_points the number of discreet points between min and max
/// \return an error code indicating the success of the operation
///
/// The \a aa_define_sensitivity_fx routine is the Q16.16 fixed point
/// version of the @ref aa_define_sensitivity routine.  All points there
/// apply equally to aa_define_sensitivity_fx.
AA_ERROR_T aa_define_sensitivity_fx( cl_instance_t cl_instance, const aa_fix16_t min, const aa_fix16_t max, const aa_size_t nbr_points );


/// \brief Set the sensitivity with the sensitivity scale for the given sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the sound pack identifier to set the sensitivity on
/// \param sensitivity the new sensitivity setting
/// \return an error code indicating the success of the operation.
///
/// The \a aa_set_sensitivity_for_soundpack_fx routine is the Q16.16 fixed
/// point version of the @ref aa_set_sensitivity_for_soundpack routine.  All
/// points there apply equally to aa_set_sensitivity_for_soundpack_fx
AA_ERROR_T aa_set_sensitivity_for_soundpack_fx( cl_instance_t cl_instance, const aa_spid_t spid, const aa_fix16_t sensitivity );



/// \brief Get the default sensitivity for a given sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the sound pack identifier which to get the sensitivity 
/// \return the default sensitivity for the given sound pack
///
/// The \a aa_get_default_sensitivity_for_soundpack_fx routine is the Q16.16
/// fixed point version of the @ref aa_get_default_sensitivity_for_soundpack
/// routine and the same points apply here.
///
/// The value returned in case the sound pack identifier is not found is
/// @ref AA_FIX16_MAX
aa_fix16_t aa_get_default_sensitivity_for_soundpack_fx( cl_instance_t cl_instance, const aa_spid_t spid);


/// \brief Get the current sensitivity for a given sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the sound pack identifier which to get the sensitivity 
/// \return the current sensitivity setting for the given sound pack
///
/// The \a aa_get_current_sensitivity_for_soundpack_fx routine is the Q16.16
/// fixed point version of the @ref aa_get_current_sensitivity_for_soundpack
/// routine and the same points apply here.
///
/// The value returned in case the sound pack identifier is not found is
/// @ref AA_FIX16_MAX
aa_fix16_t aa_get_current_sensitivity_for_soundpack_fx( cl_instance_t cl_instance, const aa_spid_t spid);


/// \brief Set the trigger level for the sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the identifier of the sound pack to alter
/// \param level the deciBel level to set as the trigger level
/// \return an error code indicating the success of the operation
///
/// The \a aa_set_trigger_level routine allows the caller to adjust the
/// trigger level of the sound pack.  Normally the CoreLogger engine
/// sets the trigger level to -90dB by default.  Using this routine
/// it's possible to adjust the level to a new level.
///
/// The trigger level may not be greater than 0 and should not normally
/// be set lower than -90.0f as the likelihood is of spurious or missed
/// detections if the level is reduced significantly.
///
/// See @ref aa_get_audio_levels to find the audio levels from a given
/// sample.
AA_ERROR_T aa_set_trigger_level( cl_instance_t cl_instance, const aa_spid_t spid, const float level );


/// \brief Set the trigger level for the sound pack
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param spid the identifier of the sound pack to alter
/// \param level the deciBel level to set as the trigger level
/// \return an error code indicating the success of the operation
///
/// The \a aa_set_trigger_level_fx routine is the Q16.16 fixed point version
/// of the @ref aa_set_trigger_level routine and the comments there apply
/// here.
AA_ERROR_T aa_set_trigger_level_fx( cl_instance_t cl_instance, const aa_spid_t spid, const aa_fix16_t level );


/// \brief Reset the frame buffer
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return an error indicating the success of the operation
///
/// The \a aa_reset_frame_buffer routine informs the CoreLogger engine
/// that the audio source has changed an that a new stream is being delivered.
///
/// When the audio source changes or is somehow switch on or off the audio
/// stream may include various transient signals that cause spikes or other
/// undesirable artifacts in the audio.  CoreLogger cannot know, without 
/// being told, of the change and so may well detect spurious events
/// in this case.
///
/// Calling this routine will reset various counters so the buffer_length
/// parameter of @ref aa_model_info_t need to be reset before any genuine
/// events will be detected.
///
/// This routine should be called whenever the audio source changes.
AA_ERROR_T aa_reset_frame_buffer( cl_instance_t cl_instance );

AA_ERROR_T aa_close_soundpack(cl_instance_t id, int index);

/// \brief Enable logging
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \param filename the file name to write the log to
/// \return an error code reflecting the success of the operation
///
/// In order to validate integrations or assess performance it is possible
/// to tell CoreLogger to produce a log of what it is doing.  This log
/// contains amongst other things, the models loaded, the raw audio passed
/// to CoreLogger via @ref aa_classify_sound_frame as well as the scores
/// for each of the models currently active.
/// 
/// AudioAnalytic has tools to post process this data and be able to replay
/// specific audio data and check the score generation.
///
/// Note that calling \a aa_enable_logging when logging is already enabled
/// will result in an error code being returned indiciating the log is already
/// active.
///
/// \warning The log file will grow quickly.  The calculation for determining
/// space is approximately (2*sample_rate+number_of_models*4)*time_in_seconds.
/// Additionally if enabled on multiple CoreLogger instances the number of
/// currently logging instance should be factored by multiplying the above value
/// by the number of logging instances.
AA_ERROR_T aa_enable_logging( cl_instance_t cl_instance, const char *filename );


/// \brief Close logging
/// \param cl_instance a valid CoreLogger instance created with
/// @ref aa_new_instance
/// \return an error code reflecting the success of the operation
///
/// Stop logging and close the logging file on the CoreLogger instance
/// supplied.
///
/// Returns success if there is no log open.
AA_ERROR_T aa_close_logging( cl_instance_t cl_instance );



/// \brief Retrieve library version
/// \return an unsigned value representing the version of the library
///
/// The \a aa_version routine returns the version of the library.
/// The version is coded as the major version in the top 16 bits and the
/// minor version in the low 16 bits.
///
/// Note that the value returned by \a aa_version must match the version
/// of the sound packs and models in order to be able to load the models.
unsigned int aa_version( void );

#ifdef __cplusplus
}
#endif

#endif
