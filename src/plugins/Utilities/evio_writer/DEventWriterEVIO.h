#ifndef _DEventWriterEVIO_
#define _DEventWriterEVIO_

#include <map>
#include <vector>
#include <string>

#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/JApplication.h>
#include <JANA/Services/JLockService.h>

#include <DAQ/JEventSource_EVIO.h>
#include <DAQ/JEventSource_EVIOpp.h>

#include <pthread.h>
#include <stdint.h>
#include <fstream>

#include <JANA/JEvent.h>

#include <DAQ/Df250PulseIntegral.h>
#include <DAQ/Df250TriggerTime.h>
#include <DAQ/Df250WindowRawData.h>
#include <DAQ/Df125PulseIntegral.h>
#include <DAQ/Df125TriggerTime.h>
#include <DAQ/Df125WindowRawData.h>
#include <DAQ/Df125CDCPulse.h>
#include <DAQ/Df125FDCPulse.h>
#include <DAQ/Df125Config.h>
#include <DAQ/DF1TDCTriggerTime.h>
#include <DAQ/DF1TDCHit.h>
#include <DAQ/DF1TDCConfig.h>
#include <DAQ/DCAEN1290TDCConfig.h>
#include <DAQ/DCAEN1290TDCHit.h>
#include <DAQ/DEPICSvalue.h>
#include <DAQ/DEventTag.h>

#include <DANA/DStatusBits.h>
#include <TTAB/DTranslationTable.h>

#include "HDEVIOWriter.h"
#include "DEVIOBufferWriter.h"

using namespace std;

class DEventWriterEVIO : public JObject
{
	public:
		JOBJECT_PUBLIC(DEventWriterEVIO);

		DEventWriterEVIO(const std::shared_ptr<const JEvent>& locEvent);
		~DEventWriterEVIO(void);

		bool Write_EVIOEvent(const std::shared_ptr<const JEvent>& locEvent, string locOutputFileNameSubString) const;
		bool Write_EVIOEvent(const std::shared_ptr<const JEvent>& locEvent, string locOutputFileNameSubString,
                             vector<const JObject *> &locObjectsToSave) const;
		bool Write_EVIOBuffer(const std::shared_ptr<const JEvent>& locEvent, vector<uint32_t> *locOutputBuffer, string locOutputFileNameSubString) const;
		bool Write_EVIOBuffer(const std::shared_ptr<const JEvent>& locEvent, uint32_t *locOutputBuffer, uint32_t locOutputBufferSize, string locOutputFileNameSubString) const;

		string Get_OutputFileName(const std::shared_ptr<const JEvent>& locEvent, string locOutputFileNameSubString) const;
        void SetDetectorsToWriteOut(const std::shared_ptr<const JEvent>& locEvent, string detector_list, string locOutputFileNameSubString) const;

        bool Is_MergingFiles() const { return dMergeFiles; }
        void Set_MergeFiles(bool in_flag) { dMergeFiles = in_flag; }
        string Get_MergedFilename() const { return dMergedFilename; }
        void Set_MergedFilename(string in_filename) { dMergedFilename = in_filename; }

		bool COMPACT;
		bool PREFER_EMULATED;
		bool DEBUG_FILES;
        bool CLOSE_FILES;
        
	protected:
		bool Open_OutputFile(const std::shared_ptr<const JEvent>& locEvent, string locOutputFileName) const;
		
		std::ofstream *ofs_debug_input;
		std::ofstream *ofs_debug_output;

        // the Translation Table is needed to get the mapping of detector type to ROC number
        const DTranslationTable *ttab;

        bool dMergeFiles;
        string dMergedFilename;

	private:

		std::shared_ptr<JLockService> lockService;

		//contain static variables shared amongst threads: acquire "EVIOWriter" write lock before calling
		size_t& Get_NumEVIOOutputThreads(void) const;
		map<string, HDEVIOWriter*>& Get_EVIOOutputters(void) const;
		map<string, pthread_t>& Get_EVIOOutputThreads(void) const;
        map<string, DEVIOBufferWriter*>& Get_EVIOBufferWriters(void) const;
};

#endif //_DEventWriterEVIO_

