/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */
#include "srslte/phy/phch/csi.h"
#include "srslte/phy/utils/bit.h"
#include "srslte/phy/utils/debug.h"
#include <math.h>

#define CSI_WIDEBAND_CSI_NOF_BITS 4

/// Implements SNRI to CQI conversion
uint32_t csi_snri_db_to_cqi(srslte_csi_cqi_table_t table, float snri_db)
{
  return 15;
}

// Implements CSI report triggers
static bool csi_report_trigger(const srslte_csi_hl_report_cfg_t* cfg, uint32_t slot_idx)
{
  switch (cfg->type) {
    case SRSLTE_CSI_REPORT_TYPE_PERIODIC:
      return (slot_idx + cfg->periodic.period - cfg->periodic.offset) % cfg->periodic.period == 0;
    default:; // Do nothing
  }
  return false;
}

static void csi_wideband_cri_ri_pmi_cqi_quantify(const srslte_csi_hl_report_cfg_t* cfg,
                                                 const srslte_csi_measurements_t*  channel_meas,
                                                 const srslte_csi_measurements_t*  interf_meas,
                                                 srslte_csi_report_cfg_t*          report_cfg,
                                                 srslte_csi_report_value_t*        report_value)
{
  // Take SNR by default
  float wideband_sinr_db = channel_meas->wideband_snr_db;

  // If interference is provided, use the channel RSRP and interference EPRE to calculate the SINR
  if (interf_meas != NULL) {
    wideband_sinr_db = channel_meas->wideband_rsrp_dBm - interf_meas->wideband_epre_dBm;
  }

  // Fill report configuration
  report_cfg->type      = cfg->type;
  report_cfg->quantity  = SRSLTE_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI;
  report_cfg->freq_cfg  = SRSLTE_CSI_REPORT_FREQ_WIDEBAND;
  report_cfg->nof_ports = channel_meas->nof_ports;
  report_cfg->K_csi_rs  = channel_meas->K_csi_rs;

  // Save PUCCH resource only if periodic type
  if (cfg->type == SRSLTE_CSI_REPORT_TYPE_PERIODIC) {
    report_cfg->pucch_resource = cfg->periodic.resource;
  }

  // Fill quantified values
  report_value->wideband_cri_ri_pmi_cqi.cqi = csi_snri_db_to_cqi(cfg->cqi_table, wideband_sinr_db);
  report_value->wideband_cri_ri_pmi_cqi.ri  = 0;
  report_value->wideband_cri_ri_pmi_cqi.pmi = 0;
}

static uint32_t csi_wideband_cri_ri_pmi_cqi_nof_bits(const srslte_csi_report_cfg_t* cfg)
{
  // Avoid K_csi_rs invalid value
  if (cfg->K_csi_rs == 0) {
    ERROR("Invalid K_csi_rs=%d", cfg->K_csi_rs);
    return 0;
  }

  // Compute number of bits for CRI
  uint32_t nof_bits_cri = (uint32_t)ceilf(log2f((float)cfg->K_csi_rs));

  switch (cfg->nof_ports) {
    case 1:
      return SRSLTE_CSI_REPORT_FREQ_WIDEBAND + nof_bits_cri;
    default:
      ERROR("Invalid or not implemented number of ports (%d)", cfg->nof_ports);
  }
  return 0;
}

static int csi_wideband_cri_ri_pmi_cqi_pack(const srslte_csi_report_cfg_t*   cfg,
                                            const srslte_csi_report_value_t* value,
                                            uint8_t*                         o_csi1)
{
  // Avoid K_csi_rs invalid value
  if (cfg->K_csi_rs == 0) {
    ERROR("Invalid K_csi_rs=%d", cfg->K_csi_rs);
    return SRSLTE_ERROR;
  }

  // Write wideband CQI
  srslte_bit_unpack(value->wideband_cri_ri_pmi_cqi.cqi, &o_csi1, CSI_WIDEBAND_CSI_NOF_BITS);

  // Compute number of bits for CRI and write
  uint32_t nof_bits_cri = (uint32_t)ceilf(log2f((float)cfg->K_csi_rs));
  srslte_bit_unpack(value->cri, &o_csi1, nof_bits_cri);

  return nof_bits_cri + CSI_WIDEBAND_CSI_NOF_BITS;
}

int srslte_csi_generate_reports(const srslte_csi_hl_cfg_t*      cfg,
                                uint32_t                        slot_idx,
                                const srslte_csi_measurements_t measurements[SRSLTE_CSI_MAX_NOF_RESOURCES],
                                srslte_csi_report_cfg_t         report_cfg[SRSLTE_CSI_MAX_NOF_REPORT],
                                srslte_csi_report_value_t       report_value[SRSLTE_CSI_MAX_NOF_REPORT])
{
  uint32_t count = 0;

  // Check inputs
  if (cfg == NULL || measurements == NULL || report_cfg == NULL || report_value == NULL) {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  // Iterate every possible configured CSI report
  for (uint32_t i = 0; i < SRSLTE_CSI_MAX_NOF_REPORT; i++) {
    // Skip if report is not configured or triggered
    if (!csi_report_trigger(&cfg->reports[i], slot_idx)) {
      continue;
    }

    // Select channel measurement
    if (cfg->reports->channel_meas_id >= SRSLTE_CSI_MAX_NOF_RESOURCES) {
      ERROR("Channel measurement ID (%d) is out of range", cfg->reports->channel_meas_id);
      return SRSLTE_ERROR;
    }
    const srslte_csi_measurements_t* channel_meas = &measurements[cfg->reports->channel_meas_id];

    // Select interference measurement
    const srslte_csi_measurements_t* interf_meas = NULL;
    if (cfg->reports->interf_meas_present) {
      if (cfg->reports->interf_meas_id >= SRSLTE_CSI_MAX_NOF_RESOURCES) {
        ERROR("Interference measurement ID (%d) is out of range", cfg->reports->interf_meas_id);
        return SRSLTE_ERROR;
      }
      interf_meas = &measurements[cfg->reports->interf_meas_id];
    }

    // Quantify measurements according to frequency and quantity configuration
    if (cfg->reports->freq_cfg == SRSLTE_CSI_REPORT_FREQ_WIDEBAND &&
        cfg->reports->quantity == SRSLTE_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI) {
      csi_wideband_cri_ri_pmi_cqi_quantify(
          &cfg->reports[i], channel_meas, interf_meas, &report_cfg[count], &report_value[count]);
      count++;
    } else {
      ; // Ignore other types
    }
  }

  return (int)count;
}

int srslte_csi_nof_bits(const srslte_csi_report_cfg_t* report_list, uint32_t nof_reports)
{
  uint32_t count = 0;

  // Check input pointer
  if (report_list == NULL) {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  // Iterate all report configurations
  for (uint32_t i = 0; i < nof_reports; i++) {
    const srslte_csi_report_cfg_t* report = &report_list[i];
    if (report->quantity && report->quantity == SRSLTE_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI) {
      count += csi_wideband_cri_ri_pmi_cqi_nof_bits(report);
    }
  }

  return (int)count;
}

int srslte_csi_part1_pack(const srslte_csi_report_cfg_t*   report_cfg,
                          const srslte_csi_report_value_t* report_value,
                          uint32_t                         nof_reports,
                          uint8_t*                         o_csi1,
                          uint32_t                         max_o_csi1)
{
  uint32_t count = 0;

  if (report_cfg == NULL || report_value == NULL || o_csi1 == NULL) {
    return SRSLTE_ERROR_INVALID_INPUTS;
  }

  int n = srslte_csi_nof_bits(report_cfg, nof_reports);
  if (n > (int)max_o_csi1) {
    ERROR("The maximum number of CSI bits (%d) is not enough to accommodate %d bits", max_o_csi1, n);
    return SRSLTE_ERROR;
  }

  for (uint32_t i = 0; i < nof_reports && count < max_o_csi1; i++) {
    if (report_cfg[i].freq_cfg == SRSLTE_CSI_REPORT_FREQ_WIDEBAND &&
        report_cfg[i].quantity == SRSLTE_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI) {
      count += csi_wideband_cri_ri_pmi_cqi_pack(&report_cfg[i], &report_value[i], &o_csi1[count]);
    } else {
      ERROR("CSI frequency (%d) and quantity (%d) combination is not implemented",
            report_cfg[i].freq_cfg,
            report_cfg[i].quantity);
    }
  }

  return (int)count;
}

uint32_t srslte_csi_str(const srslte_csi_report_cfg_t*   report_cfg,
                        const srslte_csi_report_value_t* report_value,
                        uint32_t                         nof_reports,
                        char*                            str,
                        uint32_t                         str_len)
{
  return srslte_print_check(str, str_len, 0, "cqi=%s", report_value->wideband_cri_ri_pmi_cqi.cqi);
}