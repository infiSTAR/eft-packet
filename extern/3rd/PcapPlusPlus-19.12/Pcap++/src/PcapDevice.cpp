#include "PcapDevice.h"
#include "PcapFilter.h"
#include "Logger.h"
#include <pcap.h>

namespace pcpp
{

IPcapDevice::~IPcapDevice()
{
}

bool IPcapDevice::setFilter(std::string filterAsString)
{
	LOG_DEBUG("Filter to be set: '%s'", filterAsString.c_str());
	if (!m_DeviceOpened)
	{
		LOG_ERROR("Device not Opened!! cannot set filter");
		return false;
	}

	struct bpf_program prog;
	LOG_DEBUG("Compiling the filter '%s'", filterAsString.c_str());
	if (pcap_compile(m_PcapDescriptor, &prog, filterAsString.c_str(), 1, 0) < 0)
	{
		/*
		* Print out appropriate text, followed by the error message
		* generated by the packet capture library.
		*/
		LOG_ERROR("Error compiling filter. Error message is: %s", pcap_geterr(m_PcapDescriptor));
		pcap_freecode(&prog);
		return false;
	}

	LOG_DEBUG("Setting the compiled filter");
	if (pcap_setfilter(m_PcapDescriptor, &prog) < 0)
	{
		/*
		 * Print out error. The format will be the prefix string,
		 * created above, followed by the error message that the packet
		 * capture library generates.
		 */
		LOG_ERROR("Error setting a compiled filter. Error message is: %s", pcap_geterr(m_PcapDescriptor));
		pcap_freecode(&prog);
		return false;
	}

	LOG_DEBUG("Filter set successfully");

	pcap_freecode(&prog);

	return true;
}

bool IPcapDevice::clearFilter()
{
	return setFilter("");
}

bool IPcapDevice::verifyFilter(std::string filterAsString)
{
	struct bpf_program prog;
	LOG_DEBUG("Compiling the filter '%s'", filterAsString.c_str());
	if (pcap_compile_nopcap(9000, pcpp::LINKTYPE_ETHERNET, &prog, filterAsString.c_str(), 1, 0) < 0)
	{
		return false;
	}

	return true;
}

bool IPcapDevice::matchPacketWithFilter(std::string filterAsString, RawPacket* rawPacket)
{
	static std::string curFilter = "";
	static struct bpf_program prog;
	if (curFilter != filterAsString)
	{
		LOG_DEBUG("Compiling the filter '%s'", filterAsString.c_str());
		pcap_freecode(&prog);
		if (pcap_compile_nopcap(9000, pcpp::LINKTYPE_ETHERNET, &prog, filterAsString.c_str(), 1, 0) < 0)
		{
			return false;
		}

		curFilter = filterAsString;
	}

	struct pcap_pkthdr pktHdr;
	pktHdr.caplen = rawPacket->getRawDataLen();
	pktHdr.len = rawPacket->getRawDataLen();
	pktHdr.ts = rawPacket->getPacketTimeStamp();

	return (pcap_offline_filter(&prog, &pktHdr, rawPacket->getRawData()) != 0);
}

bool IPcapDevice::matchPacketWithFilter(GeneralFilter& filter, RawPacket* rawPacket)
{
	return filter.matchPacketWithFilter(rawPacket);
}

std::string IPcapDevice::getPcapLibVersionInfo()
{
	return std::string(pcap_lib_version());
}

} // namespace pcpp
