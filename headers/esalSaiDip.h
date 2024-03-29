/*
 * esalSaidip.h
 *
 * copyright (C) 2021 Fujitsu Network Communications, Inc.
 *
 *  Created on: May 12, 2023
 */
#ifndef ESAL_VENDOR_API_HEADERS_ESALSAIDIP_H
#define ESAL_VENDOR_API_HEADERS_ESALSAIDIP_H
#ifndef LARCH_ENVIRON

#include <string>
#include <vector>
#include <dip/dip.h>

#define ESALSAI_DIP_CLASS(DipClass)                                          \
  class EsalSai##DipClass : public DipFsEntry {                              \
   public:                                                                   \
    virtual ~EsalSai##DipClass() {}                                          \
    EsalSai##DipClass(const std::string& path, const std::string& help_line, \
                   std::shared_ptr<DipCommand> cmd, void*)                   \
        : DipFsEntry(path, help_line),                                       \
          cmd_(cmd) {}                                                       \
    virtual void dip_handle_cmd(const std::string& path,                     \
                                const std::vector<std::string>& args);       \
                                                                             \
   private:                                                                  \
    std::shared_ptr<DipCommand> cmd_;                                        \
  }

  ESALSAI_DIP_CLASS(DipEsalHealthMon); 
  ESALSAI_DIP_CLASS(DipEsalPolicerStats); 
  ESALSAI_DIP_CLASS(DipEsalClearPolicerStats);
  ESALSAI_DIP_CLASS(DipEsalDumpSfp);

class EsalSaiDips {
 public:
  explicit EsalSaiDips()
      : esalsai_dip_(std::make_shared<DipCommand>()),
        esalHealthMon_("esalsai/esalHealthMon",
                        "esalHealthMon enable|disable",
                        esalsai_dip_, nullptr),
        esalPolicerStats_("esalsai/esalPolicerStats",
                        "esalPolicerStats lPort",
                        esalsai_dip_, nullptr),
        esalClearPolicerStats_("esalsai/esalClearPolicerStats",
                        "esalClearPolicerStats lPort",
                        esalsai_dip_, nullptr),
        esalDumpSfp_("esalsai/esalDumpSfp",
                        "esalDumpSfp lPort",
                        esalsai_dip_, nullptr)
{
  esalsai_dip_->dip_register_command(&esalHealthMon_);
  esalsai_dip_->dip_register_command(&esalPolicerStats_);
  esalsai_dip_->dip_register_command(&esalClearPolicerStats_);
  esalsai_dip_->dip_register_command(&esalDumpSfp_);
}
protected:
  std::shared_ptr<DipCommand> esalsai_dip_;

 private:
  EsalSaiDipEsalHealthMon           esalHealthMon_;
  EsalSaiDipEsalPolicerStats        esalPolicerStats_;
  EsalSaiDipEsalClearPolicerStats   esalClearPolicerStats_;
  EsalSaiDipEsalDumpSfp             esalDumpSfp_;
};
#endif
#endif //ESAL_VENDOR_API_HEADERS_ESALSAIDIP_H
