/*
 * ui_crypto_dashboard.h - tela "Crypto Dashboard" (LVGL 9) para ESP32 + ILI9341 320x240
 *
 * Contém: paleta, fontes, moedas, layout, classe CryptoDashboardUI (header, cards, rodapé
 * com botões, WebSocket Binance, fallback CoinGecko) e as funções de entrada:
 *   create_crypto_screen(lv_obj_t* scr);   // monta a tela (chamar depois de lv_init + display)
 *   crypto_loop();                         // chamar em todo loop()
 *   crypto_set_battery(int percent);       // opcional (negativo esconde o ícone)
 *
 * Os valores de configuração abaixo podem ser sobrescritos com #define no .ino,
 * ANTES do #include "ui_crypto_dashboard.h".
 * Requer: lvgl 9.x, WebSockets (Markus Sattler), ArduinoJson 7.x, C++17 (core esp32 3.x).
 */
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>
#include <lvgl.h>
#include "UITheme.h"

// ===================== Configuração (pode ser sobrescrita no .ino antes do #include) =====================
#ifndef LCD_W
#define LCD_W 320                   // paisagem
#endif
#ifndef LCD_H
#define LCD_H 240
#endif
#ifndef CRYPTO_TZ
#define CRYPTO_TZ            "<-03>3"   // America/Sao_Paulo (UTC-3)
#endif
#ifndef CRYPTO_DEFAULT_TF
#define CRYPTO_DEFAULT_TF    0          // timeframe inicial: 0=1m 1=5m 2=15m 3=1h
#endif
#ifndef CRYPTO_POLL_MS
#define CRYPTO_POLL_MS       30000      // CoinGecko quando a Binance cai
#endif
#ifndef CRYPTO_AUTOSCROLL_MS
#define CRYPTO_AUTOSCROLL_MS 60000      // cada página fica 1 minuto (0 desliga a troca automática)
#endif
#ifndef CD_BORDER_OPA
#define CD_BORDER_OPA        LV_OPA_60  // opacidade da borda dos cards (LV_OPA_30 = mais suave, LV_OPA_COVER = cheia)
#endif

// ===================== Fontes (usa a maior ativa no lv_conf.h) =====================
#if LV_FONT_MONTSERRAT_12
  #define CD_FONT_SM (&lv_font_montserrat_12)
#else
  #define CD_FONT_SM (&lv_font_montserrat_14)
#endif
#define CD_FONT_MD (&lv_font_montserrat_14)
#if LV_FONT_MONTSERRAT_16
  #define CD_FONT_LG (&lv_font_montserrat_16)
#else
  #define CD_FONT_LG (&lv_font_montserrat_14)
#endif

// Fontes do preço, da maior para a menor: o código usa a maior que couber no card
static const lv_font_t* const PRICE_FONTS[] = {
#if LV_FONT_MONTSERRAT_48
  &lv_font_montserrat_48,
#endif
#if LV_FONT_MONTSERRAT_40
  &lv_font_montserrat_40,
#endif
#if LV_FONT_MONTSERRAT_36
  &lv_font_montserrat_36,
#endif
#if LV_FONT_MONTSERRAT_32
  &lv_font_montserrat_32,
#endif
#if LV_FONT_MONTSERRAT_28
  &lv_font_montserrat_28,
#endif
#if LV_FONT_MONTSERRAT_24
  &lv_font_montserrat_24,
#endif
#if LV_FONT_MONTSERRAT_20
  &lv_font_montserrat_20,
#endif
#if LV_FONT_MONTSERRAT_16
  &lv_font_montserrat_16,
#endif
  &lv_font_montserrat_14
};
#define NUM_PRICE_FONTS ((int)(sizeof(PRICE_FONTS) / sizeof(PRICE_FONTS[0])))

// ===================== Moedas e timeframes =====================
#define NUM_COINS 6
struct Coin { const char* sym; const char* pair; const char* gecko; uint32_t color; };
static const Coin COINS[NUM_COINS] = {
  {"BTC",  "BTCUSDT",  "bitcoin",     0xF7931A},
  {"ETH",  "ETHUSDT",  "ethereum",    0x627EEA},
  {"SOL",  "SOLUSDT",  "solana",      0x9945FF},
  {"BNB",  "BNBUSDT",  "binancecoin", 0xF3BA2F},
  {"XRP",  "XRPUSDT",  "ripple",      0x8B98A9},
  {"DOGE", "DOGEUSDT", "dogecoin",    0xC2A633},
};

#define NUM_TF 4
static const char* const TF_NAMES[NUM_TF] = {"1m", "5m", "15m", "1h"};   // intervalos da Binance

// ===================== Layout =====================
#define CD_HEADER_H     30
#define CD_FOOTER_H     26                    // rodapé com os botões
#define CD_FOOTER_Y     (LCD_H - CD_FOOTER_H) // 214
#define CD_TOOLBAR_Y    (CD_FOOTER_Y + 3)     // botões dentro do rodapé
#define CD_TOOLBAR_H    22
#define CD_LIST_Y       32
#define CD_PAD          8                     // margem lateral
#define CD_GAP          6                     // espaço entre cards
#define CD_LIST_PADV    6
#define CD_CARD_W       (LCD_W - 2 * CD_PAD)  // 304: largura da tela
#define CD_CARD_H       82                    // 2 cards cabem na lista (182 px)
#define CD_CARD_IN_W    (CD_CARD_W - 4)       // área interna (borda de 2 px)
#define CD_CARD_IN_H    (CD_CARD_H - 4)
#define CD_CANDLES      36                    // velas por card
#define CD_PITCH        5                     // 3 px de corpo + 2 px de espaço
#define CD_CHART_W      (CD_CANDLES * CD_PITCH)   // 180
#define CD_CHART_H      28
#define CD_PRICE_MAXW   204                   // largura máxima do preço (não invade a coluna da esquerda)
#define CD_PRICE_CY     22                    // centro vertical do preço dentro do card
#define CD_PAGE_PX      (2 * (CD_CARD_H + CD_GAP))   // 176 px: uma página = 2 cards
#define CD_PAGES        (NUM_COINS / 2)

struct Candle { uint32_t t; float o, h, l, c; };   // t = abertura da vela, em segundos

struct CoinUI {
  lv_obj_t* card;
  lv_obj_t* open;
  lv_obj_t* high;
  lv_obj_t* pct;
  lv_obj_t* price;
  lv_obj_t* chart;
  int price_len;                                   // nº de caracteres do preço (reajusta a fonte ao mudar)
  Candle candles[CD_CANDLES];
  int n;
};

class CryptoDashboardUI {
private:
  lv_obj_t *lbl_clock = nullptr;
  lv_obj_t *ico_batt  = nullptr;
  lv_obj_t *ico_wifi  = nullptr;
  lv_obj_t *lbl_page  = nullptr;
  lv_obj_t *foot      = nullptr;
  lv_obj_t *btn_lock  = nullptr;
  bool page_locked = false;          // true = rotação automática parada na página atual
  lv_obj_t *btn_tf[NUM_TF] = {nullptr};
  lv_obj_t *list      = nullptr;
  lv_timer_t *auto_timer = nullptr;
  CoinUI ui[NUM_COINS];

  WebSocketsClient ws;
  JsonDocument filterDoc;
  String wsPath;
  String geckoUrl;
  bool ws_started = false;
  bool binance_down = false;
  uint32_t last_poll = 0;
  int cur_tf = CRYPTO_DEFAULT_TF;    // timeframe selecionado
  int pending_tf = -1;               // troca pedida pelo toque, aplicada no loop()

  // 4134.24 | 168.920 | 0.52814
  static void fmt_num(double v, char* out) {
    int dec = v >= 100 ? 2 : (v >= 1 ? 3 : 5);
    snprintf(out, 24, "%.*f", dec, v);
  }

  // texto escuro sobre cores claras, branco sobre cores escuras
  static lv_color_t on_color(uint32_t rgb) {
    uint32_t r = (rgb >> 16) & 255, g = (rgb >> 8) & 255, b = rgb & 255;
    return ((r * 299 + g * 587 + b * 114) / 1000 > 150) ? UITheme::BG_MAIN : UITheme::TEXT_WHITE;
  }

  int find_coin(const char* pair) {
    for (int i = 0; i < NUM_COINS; i++) if (strcmp(COINS[i].pair, pair) == 0) return i;
    return -1;
  }

  // ---------- Preço: usa a maior fonte que couber ----------
  void fit_price(CoinUI& u, const char* text) {
    lv_label_set_text(u.price, text);
    int len = (int)strlen(text);
    if (len == u.price_len) return;               // mesma quantidade de dígitos: mantém a fonte
    u.price_len = len;
    const lv_font_t* f = PRICE_FONTS[NUM_PRICE_FONTS - 1];
    for (int k = 0; k < NUM_PRICE_FONTS; k++) {
      lv_obj_set_style_text_font(u.price, PRICE_FONTS[k], 0);
      lv_obj_update_layout(u.price);
      f = PRICE_FONTS[k];
      if (lv_obj_get_width(u.price) <= CD_PRICE_MAXW) break;
    }
    lv_obj_align(u.price, LV_ALIGN_TOP_RIGHT, -8, CD_PRICE_CY - lv_font_get_line_height(f) / 2);
  }

  // ---------- Atualização de um card ----------
  void update_view(int i, double c, double o, double h) {
    CoinUI& u = ui[i];
    char num[24], buf[40];

    fmt_num(c, num);
    fit_price(u, num);

    if (o > 0) { fmt_num(o, num); snprintf(buf, sizeof(buf), "O %s", num); lv_label_set_text(u.open, buf); }
    if (h > 0) { fmt_num(h, num); snprintf(buf, sizeof(buf), "H %s", num); lv_label_set_text(u.high, buf); }
    else       { lv_label_set_text(u.high, "H --"); }

    double pct = (o > 0) ? (c - o) / o * 100.0 : 0.0;
    bool up = pct >= 0;
    snprintf(buf, sizeof(buf), "%+.2f%%", pct);
    lv_label_set_text(u.pct, buf);
    lv_obj_set_style_bg_color(u.pct, up ? CryptoTheme::UP : CryptoTheme::DOWN, 0);
    lv_obj_set_style_text_color(u.pct, up ? UITheme::BG_MAIN : UITheme::TEXT_WHITE, 0);

    // borda do card: verde em alta, vermelha em baixa (com opacidade para ficar suave)
    lv_obj_set_style_border_color(u.card, up ? CryptoTheme::UP : CryptoTheme::DOWN, 0);
    lv_obj_set_style_border_opa(u.card, CD_BORDER_OPA, 0);
  }

  // ---------- Header: hora/data e ícone de Wi-Fi ----------
  void refresh_clock() {
    char buf[32];
    time_t now = time(nullptr);
    if (now > 1700000000) {                    // só depois do NTP sincronizar
      struct tm tmv;
      localtime_r(&now, &tmv);
      snprintf(buf, sizeof(buf), "%02d/%02d %02d:%02d:%02d",
               tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
      lv_label_set_text(lbl_clock, buf);
    } else {
      lv_label_set_text(lbl_clock, "--/-- --:--:--");
    }
    // Wi-Fi: branco = Binance ao vivo, amarelo = fallback CoinGecko, vermelho = sem Wi-Fi
    bool wifi = WiFi.status() == WL_CONNECTED;
    lv_color_t col = !wifi ? CryptoTheme::DOWN : (binance_down ? CryptoTheme::WARN : UITheme::TEXT_WHITE);
    lv_obj_set_style_text_color(ico_wifi, col, 0);
  }

  static void clock_cb(lv_timer_t* t) {
    static_cast<CryptoDashboardUI*>(lv_timer_get_user_data(t))->refresh_clock();
  }

  // ---------- Páginas da lista (2 cards por página) ----------
  int cur_page() {
    int p = (lv_obj_get_scroll_y(list) + CD_PAGE_PX / 2) / CD_PAGE_PX;
    return p < 0 ? 0 : (p >= CD_PAGES ? CD_PAGES - 1 : p);
  }

  void update_page_label(int p) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d/%d", p + 1, CD_PAGES);
    lv_label_set_text(lbl_page, buf);
  }

  void go_page(int p) {
    p = ((p % CD_PAGES) + CD_PAGES) % CD_PAGES;     // dá a volta nas duas pontas
    lv_obj_scroll_to_y(list, p * CD_PAGE_PX, LV_ANIM_ON);
    update_page_label(p);
  }

  void reset_autoscroll() { if (auto_timer && !page_locked) lv_timer_reset(auto_timer); }

  // botão "n/3": trava/destrava a rotação automática (vermelho claro = travado)
  void style_lock() {
    lv_obj_set_style_bg_color(btn_lock, page_locked ? CryptoTheme::LOCK : UITheme::TOPBAR, 0);
    lv_obj_set_style_border_color(btn_lock, page_locked ? CryptoTheme::LOCK : UITheme::BORDER_ACTIVE, 0);
    lv_obj_set_style_text_color(lbl_page, page_locked ? UITheme::BG_MAIN : UITheme::TEXT_WHITE, 0);
  }

  void toggle_lock() {
    page_locked = !page_locked;
    if (auto_timer) {
      if (page_locked) lv_timer_pause(auto_timer);
      else { lv_timer_reset(auto_timer); lv_timer_resume(auto_timer); }
    }
    style_lock();
  }

  static void autoscroll_cb(lv_timer_t* t) {
    CryptoDashboardUI* self = static_cast<CryptoDashboardUI*>(lv_timer_get_user_data(t));
    self->go_page(self->cur_page() + 1);
  }

  // fim de qualquer rolagem (automática, por botão ou arrastando): atualiza "n/3" e reinicia o minuto
  static void scroll_end_cb(lv_event_t* e) {
    CryptoDashboardUI* self = static_cast<CryptoDashboardUI*>(lv_event_get_user_data(e));
    self->update_page_label(self->cur_page());
    self->reset_autoscroll();
  }

  // ---------- Botões ----------
  // ids: 0..NUM_TF-1 = timeframe | 10 = página anterior | 11 = próxima página
  static void btn_cb(lv_event_t* e) {
    CryptoDashboardUI& s = CryptoDashboardUI::instance();
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    s.reset_autoscroll();
    if (id < NUM_TF) {
      if (id != s.cur_tf) {
        s.cur_tf = id;
        s.pending_tf = id;                      // a troca pesada (rede) roda no loop(), fora do LVGL
        s.style_tf_buttons();
      }
    } else if (id == 12) {
      s.toggle_lock();
    } else if (id == 10) {
      s.go_page(s.cur_page() - 1);
    } else if (id == 11) {
      s.go_page(s.cur_page() + 1);
    }
  }

  void style_tf_buttons() {
    for (int k = 0; k < NUM_TF; k++) {
      bool sel = (k == cur_tf);
      lv_obj_set_style_bg_color(btn_tf[k], sel ? UITheme::BORDER_ACTIVE : UITheme::TOPBAR, 0);
      lv_obj_t* l = lv_obj_get_child(btn_tf[k], 0);
      lv_obj_set_style_text_color(l, sel ? UITheme::TEXT_WHITE : UITheme::TEXT_MUTED, 0);
    }
  }

  // Troca de timeframe: fecha o WebSocket, limpa as velas e deixa o loop() recarregar
  // o histórico do novo intervalo e reabrir o stream.
  void apply_timeframe() {
    pending_tf = -1;
    Serial.printf("[crypto] timeframe -> %s\n", TF_NAMES[cur_tf]);
    lv_timer_handler();                         // mostra o botão selecionado antes de bloquear na rede
    if (ws_started) { ws.disconnect(); ws_started = false; }
    binance_down = false;
    for (int i = 0; i < NUM_COINS; i++) {
      ui[i].n = 0;
      lv_obj_invalidate(ui[i].chart);
    }
    build_ws_path();
  }

  // ---------- CoinGecko (fallback): uma requisição para as 6 moedas ----------
  // Chamada bloqueante: a UI pode travar por alguns segundos durante a requisição.
  void poll_coingecko() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure();                 // sem verificação de certificado
    HTTPClient http;
    http.setConnectTimeout(3000);
    http.setTimeout(4000);
    if (!http.begin(client, geckoUrl)) return;

    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      auto body = http.getString();       // getString() trata resposta "chunked"
      JsonDocument doc;
      if (!deserializeJson(doc, body)) {
        for (int i = 0; i < NUM_COINS; i++) {
          double p   = doc[COINS[i].gecko]["usd"] | 0.0;
          double chg = doc[COINS[i].gecko]["usd_24h_change"] | 0.0;
          if (p <= 0) continue;
          double o = (chg > -99.0) ? p / (1.0 + chg / 100.0) : p;
          update_view(i, p, o, 0);        // máxima 24h não vem nesse endpoint
        }
      }
    } else {
      Serial.printf("[crypto] CoinGecko HTTP %d\n", code);
    }
    http.end();
  }

  // ---------- Velas (candles) ----------
  void on_kline(int i, uint32_t t, float o, float h, float l, float c) {
    CoinUI& u = ui[i];
    if (u.n > 0 && u.candles[u.n - 1].t == t) {
      u.candles[u.n - 1] = {t, o, h, l, c};
    } else if (u.n == 0 || t > u.candles[u.n - 1].t) {
      if (u.n < CD_CANDLES) {
        u.candles[u.n++] = {t, o, h, l, c};
      } else {
        memmove(u.candles, u.candles + 1, (CD_CANDLES - 1) * sizeof(Candle));
        u.candles[CD_CANDLES - 1] = {t, o, h, l, c};
      }
    } else {
      return;                                   // vela antiga, ignora
    }
    lv_obj_invalidate(u.chart);
  }

  // Histórico inicial das 6 moedas via REST, no timeframe atual. Reaproveita a mesma
  // conexão TLS (keep-alive) e roda com o WebSocket FECHADO, para nunca haver duas
  // conexões TLS ao mesmo tempo.
  void bootstrap_klines() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setReuse(true);
    http.setConnectTimeout(3000);
    http.setTimeout(4000);

    JsonDocument filter;                        // cada vela: [t, o, h, l, c, ...]
    for (int k = 0; k < 5; k++) filter[0][k] = true;

    for (int i = 0; i < NUM_COINS; i++) {
      String url = String("https://api.binance.com/api/v3/klines?symbol=") + COINS[i].pair +
                   "&interval=" + TF_NAMES[cur_tf] + "&limit=" + String(CD_CANDLES);
      if (http.begin(client, url)) {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
          auto body = http.getString();         // getString() trata resposta "chunked"
          JsonDocument doc;
          if (!deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
            CoinUI& u = ui[i];
            Candle tmp[CD_CANDLES];
            int n = 0;
            for (JsonVariant v : doc.as<JsonArray>()) {
              if (n >= CD_CANDLES) break;
              JsonArray row = v.as<JsonArray>();
              double t = row[0] | 0.0;
              tmp[n].t = (uint32_t)(t / 1000.0);
              tmp[n].o = (float)atof(row[1] | "0");
              tmp[n].h = (float)atof(row[2] | "0");
              tmp[n].l = (float)atof(row[3] | "0");
              tmp[n].c = (float)atof(row[4] | "0");
              if (tmp[n].c > 0) n++;
            }
            if (n > 0) {
              memcpy(u.candles, tmp, n * sizeof(Candle));
              u.n = n;
              lv_obj_invalidate(u.chart);
            }
            Serial.printf("[crypto] %s %s: %d velas\n", COINS[i].sym, TF_NAMES[cur_tf], n);
          }
        } else {
          Serial.printf("[crypto] klines %s HTTP %d\n", COINS[i].sym, code);
        }
        http.end();
      }
      lv_timer_handler();                       // mantém a tela viva entre as requisições
    }
  }

  // Desenha as velas de um card: pavio (1 px) + corpo (3 px).
  static void candles_draw_cb(lv_event_t* e) {
    CryptoDashboardUI& s = CryptoDashboardUI::instance();
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    const CoinUI& u = s.ui[idx];
    if (u.n < 1) return;

    lv_layer_t* layer = lv_event_get_layer(e);
    lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
    lv_area_t box;
    lv_obj_get_coords(obj, &box);
    const int H = lv_area_get_height(&box);

    float mn = u.candles[0].l, mx = u.candles[0].h;
    for (int i = 1; i < u.n; i++) {
      if (u.candles[i].l < mn) mn = u.candles[i].l;
      if (u.candles[i].h > mx) mx = u.candles[i].h;
    }
    float pad = (mx - mn) * 0.05f;
    if (pad <= 0) pad = (mx > 0 ? mx : 1.0f) * 0.0005f;
    mn -= pad; mx += pad;
    const float k = (H - 1) / (mx - mn);
    auto Y = [&](float v) { return box.y2 - (int)((v - mn) * k + 0.5f); };

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_width = 0;
    dsc.radius = 0;

    for (int i = 0; i < u.n; i++) {
      const Candle& c = u.candles[i];
      dsc.bg_color = (c.c >= c.o) ? CryptoTheme::CANDLE_UP : CryptoTheme::CANDLE_DN;
      int x = box.x1 + i * CD_PITCH;

      lv_area_t wick = { x + 1, Y(c.h), x + 1, Y(c.l) };
      lv_draw_rect(layer, &dsc, &wick);

      int top = Y(c.o > c.c ? c.o : c.c);
      int bot = Y(c.o > c.c ? c.c : c.o);
      if (bot <= top) bot = top;                // corpo com no mínimo 1 px
      lv_area_t body = { x, top, x + 2, bot };
      lv_draw_rect(layer, &dsc, &body);
    }
  }

  // ---------- WebSocket (Binance) ----------
  void build_ws_path() {
    wsPath = "/stream?streams=";
    for (int i = 0; i < NUM_COINS; i++) {
      String p = COINS[i].pair;
      p.toLowerCase();
      wsPath += p + "@miniTicker/" + p + "@kline_" + TF_NAMES[cur_tf];
      if (i < NUM_COINS - 1) wsPath += "/";
    }
  }

  // Stream combinado: {"stream":"...","data":{...}} com miniTicker e kline de cada moeda.
  void handle_msg(uint8_t* payload, size_t length) {
    JsonDocument doc;
    if (deserializeJson(doc, payload, length, DeserializationOption::Filter(filterDoc))) return;
    JsonObject d = doc["data"];
    const char* ev = d["e"];
    const char* s  = d["s"];
    if (!ev || !s) return;
    int i = find_coin(s);
    if (i < 0) return;

    if (strcmp(ev, "24hrMiniTicker") == 0) {
      double c = atof(d["c"] | "0");
      double o = atof(d["o"] | "0");
      double h = atof(d["h"] | "0");
      if (c > 0) update_view(i, c, o, h);
    } else if (strcmp(ev, "kline") == 0) {
      JsonObject k = d["k"];
      if (strcmp(k["i"] | "", TF_NAMES[cur_tf]) != 0) return;   // vela de outro timeframe
      double t = k["t"] | 0.0;
      float c = (float)atof(k["c"] | "0");
      if (t > 0 && c > 0) {
        on_kline(i, (uint32_t)(t / 1000.0),
                 (float)atof(k["o"] | "0"), (float)atof(k["h"] | "0"),
                 (float)atof(k["l"] | "0"), c);
      }
    }
  }

  static void ws_event(WStype_t type, uint8_t* payload, size_t length) {
    CryptoDashboardUI& self = CryptoDashboardUI::instance();
    if (type == WStype_CONNECTED) {
      Serial.println("[crypto] WebSocket conectado");
    } else if (type == WStype_DISCONNECTED || type == WStype_ERROR) {
      if (!self.binance_down && self.ws_started) Serial.println("[crypto] WebSocket indisponivel, usando CoinGecko");
      if (self.ws_started) self.binance_down = true;
    } else if (type == WStype_TEXT) {
      self.binance_down = false;
      self.handle_msg(payload, length);
    }
  }

  void start_ws() {
    ws.beginSSL("stream.binance.com", 9443, wsPath.c_str());
    ws.onEvent(ws_event);
    ws.setReconnectInterval(5000);
    ws.enableHeartbeat(15000, 3000, 2);
    ws_started = true;
  }

  // ---------- Construção da UI ----------
  lv_obj_t* make_label(lv_obj_t* parent, const char* txt, const lv_font_t* font, lv_color_t color) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
  }

  lv_obj_t* make_btn(lv_obj_t* parent, const char* txt, const lv_font_t* font,
                     int x, int y, int w, int h, int id) {
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, UITheme::TOPBAR, 0);
    lv_obj_set_style_bg_color(b, UITheme::BORDER_ACTIVE, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_border_color(b, UITheme::BORDER_ACTIVE, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_ext_click_area(b, 4);                         // área de toque um pouco maior que o botão
    lv_obj_t* l = make_label(b, txt, font, UITheme::TEXT_MUTED);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)id);
    return b;
  }

  void build_header(lv_obj_t* scr) {
    lv_obj_t* bar = lv_obj_create(scr);
    UITheme::apply_header_style(bar);
    lv_obj_set_size(bar, LCD_W, CD_HEADER_H);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    lv_obj_t* title = make_label(bar, "Crypto Dashboard", CD_FONT_LG, UITheme::TEXT_WHITE);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    lbl_clock = make_label(bar, "--/-- --:--:--", CD_FONT_SM, UITheme::TEXT_MUTED);
    lv_obj_align(lbl_clock, LV_ALIGN_RIGHT_MID, -56, 0);

    ico_batt = make_label(bar, LV_SYMBOL_BATTERY_FULL, CD_FONT_LG, UITheme::TEXT_WHITE);
    lv_obj_align(ico_batt, LV_ALIGN_RIGHT_MID, -32, 0);
    lv_obj_add_flag(ico_batt, LV_OBJ_FLAG_HIDDEN);       // só aparece após set_battery()

    ico_wifi = make_label(bar, LV_SYMBOL_WIFI, CD_FONT_LG, UITheme::TEXT_WHITE);
    lv_obj_align(ico_wifi, LV_ALIGN_RIGHT_MID, -10, 0);
  }

  // rodapé com botões: timeframes à esquerda, página anterior/próxima à direita
  void build_toolbar(lv_obj_t* scr) {
    foot = lv_obj_create(scr);
    lv_obj_set_size(foot, LCD_W, CD_FOOTER_H);
    lv_obj_set_pos(foot, 0, CD_FOOTER_Y);
    lv_obj_set_style_bg_color(foot, UITheme::TOPBAR, 0);
    lv_obj_set_style_bg_opa(foot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(foot, 0, 0);
    lv_obj_set_style_pad_all(foot, 0, 0);
    lv_obj_set_style_border_color(foot, UITheme::BORDER_IDLE, 0);
    lv_obj_set_style_border_width(foot, 1, 0);
    lv_obj_set_style_border_side(foot, LV_BORDER_SIDE_TOP, 0);
    lv_obj_remove_flag(foot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(foot, LV_OBJ_FLAG_CLICKABLE);
    const int w = 38, gap = 4;
    for (int k = 0; k < NUM_TF; k++) {
      btn_tf[k] = make_btn(foot, TF_NAMES[k], CD_FONT_SM, CD_PAD + k * (w + gap), 2, w, CD_TOOLBAR_H, k);
    }
    style_tf_buttons();

    btn_lock = make_btn(foot, "1/3", CD_FONT_SM, 180, 2, 54, CD_TOOLBAR_H, 12);
    lbl_page = lv_obj_get_child(btn_lock, 0);
    style_lock();

    make_btn(foot, LV_SYMBOL_UP,   CD_FONT_MD, LCD_W - CD_PAD - 34 * 2 - 4, 2, 34, CD_TOOLBAR_H, 10);
    make_btn(foot, LV_SYMBOL_DOWN, CD_FONT_MD, LCD_W - CD_PAD - 34,         2, 34, CD_TOOLBAR_H, 11);
  }

  void build_card(lv_obj_t* parent, int i) {
    CoinUI& u = ui[i];
    u.n = 0;
    u.price_len = 0;

    u.card = lv_obj_create(parent);
    lv_obj_set_size(u.card, CD_CARD_W, CD_CARD_H);
    lv_obj_set_style_radius(u.card, 10, 0);
    lv_obj_set_style_bg_color(u.card, UITheme::CARD_BG, 0);
    lv_obj_set_style_bg_opa(u.card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(u.card, UITheme::BORDER_IDLE, 0);   // vira verde/vermelho com os dados
    lv_obj_set_style_border_opa(u.card, CD_BORDER_OPA, 0);
    lv_obj_set_style_border_width(u.card, 2, 0);
    lv_obj_set_style_pad_all(u.card, 0, 0);
    lv_obj_remove_flag(u.card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(u.card, LV_OBJ_FLAG_SNAPPABLE);

    // nome da moeda: pílula arredondada na cor da logo
    lv_obj_t* sym = make_label(u.card, COINS[i].sym, CD_FONT_LG, on_color(COINS[i].color));
    lv_obj_set_style_bg_color(sym, lv_color_hex(COINS[i].color), 0);
    lv_obj_set_style_bg_opa(sym, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sym, 9, 0);
    lv_obj_set_style_pad_hor(sym, 9, 0);
    lv_obj_set_style_pad_ver(sym, 2, 0);
    lv_obj_set_pos(sym, 8, 3);

    u.open = make_label(u.card, "O --", CD_FONT_SM, UITheme::TEXT_MUTED);
    lv_obj_set_pos(u.open, 8, 27);
    u.high = make_label(u.card, "H --", CD_FONT_SM, UITheme::TEXT_MUTED);
    lv_obj_set_pos(u.high, 8, 41);

    // variação 24h: etiqueta verde/vermelha
    u.pct = make_label(u.card, "--", CD_FONT_MD, UITheme::TEXT_MUTED);
    lv_obj_set_style_bg_color(u.pct, UITheme::BORDER_ACTIVE, 0);
    lv_obj_set_style_bg_opa(u.pct, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(u.pct, 6, 0);
    lv_obj_set_style_pad_hor(u.pct, 6, 0);
    lv_obj_set_style_pad_ver(u.pct, 2, 0);
    lv_obj_set_pos(u.pct, 8, 56);

    // preço grande, alinhado à direita (a fonte é escolhida em fit_price)
    u.price = make_label(u.card, "--", PRICE_FONTS[0], CryptoTheme::PRICE);
    lv_obj_align(u.price, LV_ALIGN_TOP_RIGHT, -8, CD_PRICE_CY - lv_font_get_line_height(PRICE_FONTS[0]) / 2);

    // velas (desenhadas por candles_draw_cb)
    u.chart = lv_obj_create(u.card);
    lv_obj_set_size(u.chart, CD_CHART_W, CD_CHART_H);
    lv_obj_set_pos(u.chart, CD_CARD_IN_W - 8 - CD_CHART_W, CD_CARD_IN_H - CD_CHART_H - 3);
    lv_obj_set_style_bg_opa(u.chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(u.chart, 0, 0);
    lv_obj_set_style_pad_all(u.chart, 0, 0);
    lv_obj_remove_flag(u.chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(u.chart, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(u.chart, candles_draw_cb, LV_EVENT_DRAW_MAIN_END, (void*)(intptr_t)i);
  }

  void build_list(lv_obj_t* scr) {
    list = lv_obj_create(scr);
    lv_obj_set_size(list, LCD_W, CD_FOOTER_Y - CD_LIST_Y);
    lv_obj_set_pos(list, 0, CD_LIST_Y);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_pad_left(list, CD_PAD, 0);
    lv_obj_set_style_pad_right(list, CD_PAD, 0);
    lv_obj_set_style_pad_top(list, CD_LIST_PADV, 0);
    lv_obj_set_style_pad_bottom(list, CD_LIST_PADV, 0);
    lv_obj_set_style_pad_row(list, CD_GAP, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(list, LV_SCROLL_SNAP_START);

    // barra de rolagem sempre visível
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(list, CryptoTheme::BORDER_ACTIVE, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(list, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 2, LV_PART_SCROLLBAR);
    lv_obj_add_event_cb(list, scroll_end_cb, LV_EVENT_SCROLL_END, this);

    for (int i = 0; i < NUM_COINS; i++) build_card(list, i);
  }

public:
  // Forward declaration para uso no callback estático
  static CryptoDashboardUI& instance();

  void init(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, UITheme::BG_MAIN, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_header(scr);
    build_toolbar(scr);
    build_list(scr);
    lv_obj_move_foreground(foot);                          // rodapé fixo, sempre por cima da lista

    // relógio via NTP
    configTzTime(CRYPTO_TZ, "pool.ntp.org", "time.google.com");
    lv_timer_create(clock_cb, 1000, this);
    refresh_clock();

#if CRYPTO_AUTOSCROLL_MS > 0
    auto_timer = lv_timer_create(autoscroll_cb, CRYPTO_AUTOSCROLL_MS, this);
#endif

    // filtro para o ArduinoJson ler só o necessário
    filterDoc["data"]["e"] = true;
    filterDoc["data"]["s"] = true;
    filterDoc["data"]["c"] = true;
    filterDoc["data"]["o"] = true;
    filterDoc["data"]["h"] = true;
    filterDoc["data"]["k"]["i"] = true;
    filterDoc["data"]["k"]["t"] = true;
    filterDoc["data"]["k"]["o"] = true;
    filterDoc["data"]["k"]["h"] = true;
    filterDoc["data"]["k"]["l"] = true;
    filterDoc["data"]["k"]["c"] = true;

    build_ws_path();
    geckoUrl = "https://api.coingecko.com/api/v3/simple/price?ids=";
    for (int i = 0; i < NUM_COINS; i++) {
      geckoUrl += COINS[i].gecko;
      if (i < NUM_COINS - 1) geckoUrl += ",";
    }
    geckoUrl += "&vs_currencies=usd&include_24hr_change=true";
    // o WebSocket só abre no loop(), depois do histórico das velas
  }

  void loop() {
    if (pending_tf >= 0) apply_timeframe();

    if (!ws_started && WiFi.status() == WL_CONNECTED) {
      bootstrap_klines();
      start_ws();
    }
    if (ws_started) ws.loop();

    if (ws_started && binance_down && (millis() - last_poll > CRYPTO_POLL_MS)) {
      poll_coingecko();
      last_poll = millis();
    }
  }

  // Bateria (0-100). Valor negativo esconde o ícone.
  void set_battery(int percent) {
    if (!ico_batt) return;
    if (percent < 0) { lv_obj_add_flag(ico_batt, LV_OBJ_FLAG_HIDDEN); return; }
    const char* sym = percent >= 88 ? LV_SYMBOL_BATTERY_FULL
                    : percent >= 63 ? LV_SYMBOL_BATTERY_3
                    : percent >= 38 ? LV_SYMBOL_BATTERY_2
                    : percent >= 13 ? LV_SYMBOL_BATTERY_1
                                    : LV_SYMBOL_BATTERY_EMPTY;
    lv_label_set_text(ico_batt, sym);
    lv_obj_set_style_text_color(ico_batt, percent <= 15 ? CryptoTheme::DOWN : UITheme::TEXT_WHITE, 0);
    lv_obj_remove_flag(ico_batt, LV_OBJ_FLAG_HIDDEN);
  }
};

inline CryptoDashboardUI g_crypto_dashboard;

inline CryptoDashboardUI& CryptoDashboardUI::instance() { return g_crypto_dashboard; }
inline void create_crypto_screen(lv_obj_t *scr) { g_crypto_dashboard.init(scr); }
inline void crypto_loop() { g_crypto_dashboard.loop(); }
inline void crypto_set_battery(int percent) { g_crypto_dashboard.set_battery(percent); }
