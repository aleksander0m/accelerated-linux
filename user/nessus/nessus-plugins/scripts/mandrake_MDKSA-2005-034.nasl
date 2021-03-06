#
# (C) Tenable Network Security
#
# This plugin text was extracted from Mandrake Linux Security Advisory MDKSA-2005:034
#


if ( ! defined_func("bn_random") ) exit(0);
if(description)
{
 script_id(16377);
 script_version ("$Revision: 1.3 $");
 script_cve_id("CVE-2005-0173", "CVE-2005-0174", "CVE-2005-0175", "CVE-2005-0211");
 
 name["english"] = "MDKSA-2005:034: squid";
 
 script_name(english:name["english"]);
 
 desc["english"] = "
The remote host is missing the patch for the advisory MDKSA-2005:034 (squid).



More vulnerabilities were discovered in the squid server:

The LDAP handling of search filters was inadequate which could be abused to
allow logins using severial variants of a single login name, possibly bypassing
explicit access controls (CVE-2005-0173).

Minor problems in the HTTP header parsing code that could be used for cache
poisoning (CVE-2005-0174 and CVE-2005-0175).

A buffer overflow in the WCCP handling code allowed remote attackers to cause a
Denial of Service and could potentially allow for the execution of arbitrary
code by using a long WCCP packet.

The updated packages have been patched to prevent these problems.



Solution : http://wwwnew.mandriva.com/security/advisories?name=MDKSA-2005:034
Risk factor : High";



 script_description(english:desc["english"]);
 
 summary["english"] = "Check for the version of the squid package";
 script_summary(english:summary["english"]);
 
 script_category(ACT_GATHER_INFO);
 
 script_copyright(english:"This script is Copyright (C) 2005 Tenable Network Security");
 family["english"] = "Mandrake Local Security Checks";
 script_family(english:family["english"]);
 
 script_dependencies("ssh_get_info.nasl");
 script_require_keys("Host/Mandrake/rpm-list");
 exit(0);
}

include("rpm.inc");
if ( rpm_check( reference:"squid-2.5.STABLE4-2.4.100mdk", release:"MDK10.0", yank:"mdk") )
{
 security_hole(0);
 exit(0);
}
if ( rpm_check( reference:"squid-2.5.STABLE6-2.3.101mdk", release:"MDK10.1", yank:"mdk") )
{
 security_hole(0);
 exit(0);
}
if ( rpm_check( reference:"squid-2.5.STABLE3-3.6.92mdk", release:"MDK9.2", yank:"mdk") )
{
 security_hole(0);
 exit(0);
}
if (rpm_exists(rpm:"squid-", release:"MDK10.0")
 || rpm_exists(rpm:"squid-", release:"MDK10.1")
 || rpm_exists(rpm:"squid-", release:"MDK9.2") )
{
 set_kb_item(name:"CVE-2005-0173", value:TRUE);
 set_kb_item(name:"CVE-2005-0174", value:TRUE);
 set_kb_item(name:"CVE-2005-0175", value:TRUE);
 set_kb_item(name:"CVE-2005-0211", value:TRUE);
}
