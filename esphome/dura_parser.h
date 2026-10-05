#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace dura {

inline int read_varint(const std::vector<uint8_t> &data, size_t &i) {
  int value = 0;
  int shift = 0;
  while (i < data.size()) {
    uint8_t b = data[i++];
    value |= static_cast<int>(b & 0x7F) << shift;
    if ((b & 0x80) == 0) {
      break;
    }
    shift += 7;
    if (shift > 28) {
      break;
    }
  }
  return value;
}

inline bool contains(const std::vector<uint8_t> &data, const char *needle) {
  const size_t nlen = strlen(needle);
  if (nlen == 0 || data.size() < nlen) {
    return false;
  }
  for (size_t i = 0; i + nlen <= data.size(); i++) {
    if (memcmp(&data[i], needle, nlen) == 0) {
      return true;
    }
  }
  return false;
}

// True name match — avoid "hiPower" matching inside "hiPowerInput".
inline bool name_at(const std::vector<uint8_t> &data, size_t pos, const char *name, size_t nlen) {
  if (pos + nlen > data.size()) {
    return false;
  }
  if (memcmp(&data[pos], name, nlen) != 0) {
    return false;
  }
  if (pos + nlen < data.size()) {
    uint8_t c = data[pos + nlen];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      return false;
    }
  }
  return true;
}

// Duratech: ASCII name, then nested … 08 <type> 10 <varint> …
inline bool find_nested_varint(const std::vector<uint8_t> &data, const char *name, int &out, int min_v,
                               int max_v, int required_type = -1) {
  const size_t nlen = strlen(name);
  for (size_t pos = 0; pos + nlen + 4 <= data.size(); pos++) {
    if (!name_at(data, pos, name, nlen)) {
      continue;
    }
    size_t i = pos + nlen;
    size_t end = i + 96;
    if (end > data.size()) {
      end = data.size();
    }
    while (i + 3 < end) {
      if (data[i] == 0x08 && data[i + 2] == 0x10) {
        if (required_type >= 0 && data[i + 1] != static_cast<uint8_t>(required_type)) {
          i++;
          continue;
        }
        size_t j = i + 3;
        int v = read_varint(data, j);
        if (v >= min_v && v <= max_v) {
          out = v;
          return true;
        }
      }
      // Fallback alleen voor temperaturen (min_v>=50): bare 0x10 <varint>
      if (required_type < 0 && min_v >= 50 && data[i] == 0x10) {
        size_t j = i + 1;
        int v = read_varint(data, j);
        if (v >= min_v && v <= max_v) {
          out = v;
          return true;
        }
      }
      i++;
    }
  }
  return false;
}

inline bool find_field_temp_x10(const std::vector<uint8_t> &data, const char *name, float &out) {
  int raw = 0;
  // Typed values from sniffer (strict — no untyped fallback):
  // Within ~96 B after hiWaterTemperature the setpoint (type 6) also appears;
  // a loose fallback would publish setpoint as inlet after SP changes.
  // water=3, setpoint=6, ambient/outlet=2
  int type = -1;
  if (strcmp(name, "hiWaterTemperature") == 0) {
    type = 0x03;
  } else if (strcmp(name, "hiSetpoint") == 0) {
    type = 0x06;
  } else if (strcmp(name, "hiAmbientTemperature") == 0 ||
             strcmp(name, "hiWaterOutTemperature") == 0) {
    type = 0x02;
  }
  if (type < 0) {
    return false;
  }
  if (!find_nested_varint(data, name, raw, 50, 500, type)) {
    return false;
  }
  out = static_cast<float>(raw) / 10.0f;
  return true;
}

inline bool find_field_small_int(const std::vector<uint8_t> &data, const char *name, int &out) {
  return find_nested_varint(data, name, out, 0, 255, -1);
}

inline bool find_power_watts(const std::vector<uint8_t> &data, int &out) {
  // hiPowerInput → 08 03 10 <watts>. Veld 4 (0x20) is rated/max (sniffer: 10000), niet live.
  return find_nested_varint(data, "hiPowerInput", out, 0, 20000, 0x03);
}

inline bool find_power_on(const std::vector<uint8_t> &data, bool &on) {
  const char *name = "hiPower";
  const size_t nlen = strlen(name);
  for (size_t pos = 0; pos + nlen + 6 <= data.size(); pos++) {
    if (!name_at(data, pos, name, nlen)) {
      continue;
    }
    size_t i = pos + nlen;
    size_t end = i + 48;
    if (end > data.size()) {
      end = data.size();
    }
    bool saw_enum = false;
    while (i + 3 < end) {
      if (data[i] == 0x08 && data[i + 1] == 0x04) {
        saw_enum = true;
        if (data[i + 2] == 0x10) {
          size_t j = i + 3;
          on = (read_varint(data, j) == 1);
          return true;
        }
      }
      i++;
    }
    if (saw_enum) {
      on = false;
      return true;
    }
  }
  return false;
}

// Actieve geluidsmodus in heartbeat: strikt … 08 06 12 02 28 <mode>
// (1=Standaard 2=Boost 3=Slim 0x1b=Stille). Geen losse 12 02 28-fallback:
// die matcht ook pump id=5 (08 05 12 02 28) → valse "Standaard" snapback.
inline const char *mode_from_byte(uint8_t mode) {
  switch (mode) {
    case 0x01:
      return "Standaard";
    case 0x02:
      return "Boost";
    case 0x03:
      return "Slim";
    case 0x1b:
      return "Stille";
    default:
      return nullptr;
  }
}

inline const char *find_noise_mode(const std::vector<uint8_t> &data) {
  static const uint8_t pat[] = {0x08, 0x06, 0x12, 0x02, 0x28};
  for (size_t i = 0; i + 6 <= data.size(); i++) {
    if (memcmp(&data[i], pat, 5) == 0) {
      const char *m = mode_from_byte(data[i + 5]);
      if (m != nullptr) {
        return m;
      }
    }
  }
  return nullptr;
}

inline void encode_varint(int value, std::vector<uint8_t> &out) {
  while (true) {
    uint8_t b = static_cast<uint8_t>(value & 0x7F);
    value >>= 7;
    if (value != 0) {
      b |= 0x80;
    }
    out.push_back(b);
    if (value == 0) {
      break;
    }
  }
}

inline std::vector<uint8_t> make_setpoint_command(float temperature_c) {
  int raw = static_cast<int>(temperature_c * 10.0f + 0.5f);
  std::vector<uint8_t> cmd = {0x04, 0x1a, 0x09, 0x08, 0x08, 0x10, 0x08, 0x1a, 0x03, 0x18};
  encode_varint(raw, cmd);
  cmd.push_back(0x00);
  return cmd;
}

// Settings group 3 int writes (sniffer8): Interval=6, Check=7, OneTouch brightness=8
// Wire: <len> 1a <inner_len> 08 03 10 <id> 1a <val_len> 10 <varint> 00
inline std::vector<uint8_t> make_settings_int_command(uint8_t setting_id, int value) {
  std::vector<uint8_t> varint;
  encode_varint(value, varint);

  std::vector<uint8_t> value_msg;
  value_msg.push_back(0x10);
  value_msg.insert(value_msg.end(), varint.begin(), varint.end());

  std::vector<uint8_t> inner;
  inner.push_back(0x08);
  inner.push_back(0x03);
  inner.push_back(0x10);
  inner.push_back(setting_id);
  inner.push_back(0x1a);
  inner.push_back(static_cast<uint8_t>(value_msg.size()));
  inner.insert(inner.end(), value_msg.begin(), value_msg.end());

  std::vector<uint8_t> cmd;
  cmd.push_back(static_cast<uint8_t>(inner.size() + 3));
  cmd.push_back(0x1a);
  cmd.push_back(static_cast<uint8_t>(inner.size()));
  cmd.insert(cmd.end(), inner.begin(), inner.end());
  cmd.push_back(0x00);
  return cmd;
}

}  // namespace dura
