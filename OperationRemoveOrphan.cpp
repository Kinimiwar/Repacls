#include "OperationRemoveOrphan.h"
#include "InputOutput.h"
#include "Functions.h"

ClassFactory<OperationRemoveOrphan> * OperationRemoveOrphan::RegisteredFactory =
new ClassFactory<OperationRemoveOrphan>(GetCommand());

OperationRemoveOrphan::OperationRemoveOrphan(std::queue<std::wstring> & oArgList) : Operation(oArgList)
{
	// exit if there are not enough arguments to part
	std::vector<std::wstring> sSubArgs = ProcessAndCheckArgs(1, oArgList);

	// decode the passed parameter to an account name
	tDomainSid = GetSidFromName(sSubArgs[0]);

	// see if names could be resolved
	if (tDomainSid == nullptr)
	{
		// complain
		wprintf(L"ERROR: Invalid domain '%s' specified for parameter '%s'.\n", sSubArgs[0].c_str(), GetCommand().c_str());
		exit(0);
	}

	// do a reverse lookup of the name for reporting
	sDomainName = GetDomainNameFromSid(tDomainSid);

	// flag this as being an ace-level action
	AppliesToDacl = true;
	AppliesToSacl = true;
	AppliesToGroup = true;
	AppliesToOwner = true;

	// target certain parts of the security descriptor
	if (sSubArgs.size() > 1) ProcessGranularTargetting(sSubArgs[1]);
}

SidActionResult OperationRemoveOrphan::DetermineSid(WCHAR * const sSdPart, ObjectEntry & tObjectEntry, PSID const tCurrentSid, PSID & tResultantSid)
{
	// only bother doing a domain check if a domain was specified
	if (tDomainSid != NULL)
	{
		// see if this sid in the source domain
		BOOL bDomainSidsEqual = FALSE;
		if (EqualDomainSid(tCurrentSid, tDomainSid, &bDomainSidsEqual) == 0 ||
			bDomainSidsEqual == FALSE)
		{
			// no match - cease processing this instruction
			return SidActionResult::Nothing;
		}
	}

	// see if the sid is unresolvable; if it is then this is not an orphan
	bool bIsOrphan = false;
	std::wstring sSid = GetNameFromSidEx(tCurrentSid, &bIsOrphan);
	if (!bIsOrphan) return SidActionResult::Nothing;

	// update the sid in the ace
	InputOutput::AddInfo(L"Removing orphan of security identifier '" + sSid + L"' from domain '" + sDomainName + L"'", sSdPart);
	tResultantSid = NULL;
	return SidActionResult::Remove;
}
