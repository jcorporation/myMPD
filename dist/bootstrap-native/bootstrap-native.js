var BSN = function(exports) {
  "use strict";var __defProp = Object.defineProperty;
var __defNormalProp = (obj, key, value) => key in obj ? __defProp(obj, key, { enumerable: true, configurable: true, writable: true, value }) : obj[key] = value;
var __publicField = (obj, key, value) => {
  __defNormalProp(obj, typeof key !== "symbol" ? key + "" : key, value);
  return value;
};

  function _mergeNamespaces(n, m) {
    for (var i = 0; i < m.length; i++) {
      const e = m[i];
      if (typeof e !== "string" && !Array.isArray(e)) {
        for (const k in e) {
          if (k !== "default" && !(k in n)) {
            const d = Object.getOwnPropertyDescriptor(e, k);
            if (d) {
              Object.defineProperty(n, k, d.get ? d : {
                enumerable: true,
                get: () => e[k]
              });
            }
          }
        }
      }
    }
    return Object.freeze(Object.defineProperty(n, Symbol.toStringTag, { value: "Module" }));
  }
  var eventListener$1 = {};
  (function(exports2) {
    Object.defineProperty(exports2, Symbol.toStringTag, { value: "Module" });
    const r = {}, a = (s) => {
      const { type: e, currentTarget: c } = s;
      [...r[e]].forEach(([n, o]) => {
        c === n && [...o].forEach(([t, i]) => {
          t.apply(n, [s]), typeof i == "object" && i.once && f(n, e, t, i);
        });
      });
    }, d = (s, e, c, n) => {
      r[e] || (r[e] = /* @__PURE__ */ new Map());
      const o = r[e];
      o.has(s) || o.set(s, /* @__PURE__ */ new Map());
      const t = o.get(s), { size: i } = t;
      t.set(c, n), i || s.addEventListener(e, a, n);
    }, f = (s, e, c, n) => {
      const o = r[e], t = o && o.get(s), i = t && t.get(c), g = i !== void 0 ? i : n;
      t && t.has(c) && t.delete(c), o && (!t || !t.size) && o.delete(s), (!o || !o.size) && delete r[e], (!t || !t.size) && s.removeEventListener(e, a, g);
    }, E = d, L = f;
    exports2.addListener = d;
    exports2.globalListener = a;
    exports2.off = L;
    exports2.on = E;
    exports2.registry = r;
    exports2.removeListener = f;
  })(eventListener$1);
  const eventListener = /* @__PURE__ */ _mergeNamespaces({
    __proto__: null,
    default: eventListener$1
  }, [eventListener$1]);
  var shorty = {};
  (function(exports2) {
    Object.defineProperty(exports2, Symbol.toStringTag, { value: "Module" });
    const lt = "aria-checked", dt = "aria-description", mt = "aria-describedby", Et = "aria-expanded", vt = "aria-haspopup", pt = "aria-hidden", gt = "aria-label", ft = "aria-labelledby", bt = "aria-modal", yt = "aria-pressed", ht = "aria-selected", At = "aria-valuemin", wt = "aria-valuemax", kt = "aria-valuenow", St = "aria-valuetext", Q = "abort", q = "beforeunload", G = "blur", K = "change", J = "contextmenu", x = "DOMContentLoaded", X = "DOMMouseScroll", Y = "error", Z = "focus", $ = "focusin", _ = "focusout", ee = "gesturechange", te = "gestureend", ne = "gesturestart", oe = "keydown", se = "keypress", re = "keyup", ae = "load", ce = "click", ie = "dblclick", ue = "mousedown", le = "mouseup", de = "hover", me = "mouseenter", Ee = "mouseleave", ve = "mousein", pe = "mouseout", ge = "mouseover", fe = "mousemove", be = "mousewheel", ye = "move", he = "orientationchange", Ae = "pointercancel", we = "pointerdown", ke = "pointerleave", Se = "pointermove", De = "pointerup", Me = "readystatechange", Ne = "reset", Te = "resize", Ce = "select", Le = "selectend", Oe = "selectstart", Ie = "scroll", ze = "submit", xe = "touchstart", He = "touchmove", Be = "touchcancel", Ve = "touchend", Pe = "unload", Dt = { DOMContentLoaded: x, DOMMouseScroll: X, abort: Q, beforeunload: q, blur: G, change: K, click: ce, contextmenu: J, dblclick: ie, error: Y, focus: Z, focusin: $, focusout: _, gesturechange: ee, gestureend: te, gesturestart: ne, hover: de, keydown: oe, keypress: se, keyup: re, load: ae, mousedown: ue, mousemove: fe, mousein: ve, mouseout: pe, mouseenter: me, mouseleave: Ee, mouseover: ge, mouseup: le, mousewheel: be, move: ye, orientationchange: he, pointercancel: Ae, pointerdown: we, pointerleave: ke, pointermove: Se, pointerup: De, readystatechange: Me, reset: Ne, resize: Te, scroll: Ie, select: Ce, selectend: Le, selectstart: Oe, submit: ze, touchcancel: Be, touchend: Ve, touchmove: He, touchstart: xe, unload: Pe }, Mt = "drag", Nt = "dragstart", Tt = "dragenter", Ct = "dragleave", Lt = "dragover", Ot = "dragend", It = "loadstart", zt = { start: "mousedown", end: "mouseup", move: "mousemove", cancel: "mouseleave" }, xt = { down: "mousedown", up: "mouseup" }, Ht = "onmouseleave" in document ? ["mouseenter", "mouseleave"] : ["mouseover", "mouseout"], Bt = { start: "touchstart", end: "touchend", move: "touchmove", cancel: "touchcancel" }, Vt = { in: "focusin", out: "focusout" }, Pt = { Backspace: "Backspace", Tab: "Tab", Enter: "Enter", Shift: "Shift", Control: "Control", Alt: "Alt", Pause: "Pause", CapsLock: "CapsLock", Escape: "Escape", Scape: "Space", ArrowLeft: "ArrowLeft", ArrowUp: "ArrowUp", ArrowRight: "ArrowRight", ArrowDown: "ArrowDown", Insert: "Insert", Delete: "Delete", Meta: "Meta", ContextMenu: "ContextMenu", ScrollLock: "ScrollLock" }, Ft = "Alt", Rt = "ArrowDown", Wt = "ArrowUp", Ut = "ArrowLeft", jt = "ArrowRight", Qt = "Backspace", qt = "CapsLock", Gt = "Control", Kt = "Delete", Jt = "Enter", Xt = "Escape", Yt = "Insert", Zt = "Meta", $t = "Pause", _t = "ScrollLock", en = "Shift", tn = "Space", nn = "Tab", Fe = "animationDuration", Re = "animationDelay", H = "animationName", T = "animationend", We = "transitionDuration", Ue = "transitionDelay", C = "transitionend", B = "transitionProperty", on = "addEventListener", sn = "removeEventListener", rn = { linear: "linear", easingSinusoidalIn: "cubic-bezier(0.47,0,0.745,0.715)", easingSinusoidalOut: "cubic-bezier(0.39,0.575,0.565,1)", easingSinusoidalInOut: "cubic-bezier(0.445,0.05,0.55,0.95)", easingQuadraticIn: "cubic-bezier(0.550,0.085,0.680,0.530)", easingQuadraticOut: "cubic-bezier(0.250,0.460,0.450,0.940)", easingQuadraticInOut: "cubic-bezier(0.455,0.030,0.515,0.955)", easingCubicIn: "cubic-bezier(0.55,0.055,0.675,0.19)", easingCubicOut: "cubic-bezier(0.215,0.61,0.355,1)", easingCubicInOut: "cubic-bezier(0.645,0.045,0.355,1)", easingQuarticIn: "cubic-bezier(0.895,0.03,0.685,0.22)", easingQuarticOut: "cubic-bezier(0.165,0.84,0.44,1)", easingQuarticInOut: "cubic-bezier(0.77,0,0.175,1)", easingQuinticIn: "cubic-bezier(0.755,0.05,0.855,0.06)", easingQuinticOut: "cubic-bezier(0.23,1,0.32,1)", easingQuinticInOut: "cubic-bezier(0.86,0,0.07,1)", easingExponentialIn: "cubic-bezier(0.95,0.05,0.795,0.035)", easingExponentialOut: "cubic-bezier(0.19,1,0.22,1)", easingExponentialInOut: "cubic-bezier(1,0,0,1)", easingCircularIn: "cubic-bezier(0.6,0.04,0.98,0.335)", easingCircularOut: "cubic-bezier(0.075,0.82,0.165,1)", easingCircularInOut: "cubic-bezier(0.785,0.135,0.15,0.86)", easingBackIn: "cubic-bezier(0.6,-0.28,0.735,0.045)", easingBackOut: "cubic-bezier(0.175,0.885,0.32,1.275)", easingBackInOut: "cubic-bezier(0.68,-0.55,0.265,1.55)" }, an = "offsetHeight", cn = "offsetWidth", un = "scrollHeight", ln = "scrollWidth", dn = "tabindex", mn = navigator.userAgentData, A = mn, { userAgent: En } = navigator, w = En, R = /iPhone|iPad|iPod|Android/i;
    let I = false;
    A ? I = A.brands.some((e) => R.test(e.brand)) : I = R.test(w);
    const vn = I, W = /(iPhone|iPod|iPad)/, pn = A ? A.brands.some((e) => W.test(e.brand)) : W.test(w), gn = w ? w.includes("Firefox") : false, { head: k } = document, fn = ["webkitPerspective", "perspective"].some((e) => e in k.style), je = (e, t, n, o) => {
      const s = o || false;
      e.addEventListener(t, n, s);
    }, Qe = (e, t, n, o) => {
      const s = o || false;
      e.removeEventListener(t, n, s);
    }, qe = (e, t, n, o) => {
      const s = (a) => {
        (a.target === e || a.currentTarget === e) && (n.apply(e, [a]), Qe(e, t, s, o));
      };
      je(e, t, s, o);
    }, Ge = () => {
    }, bn = (() => {
      let e = false;
      try {
        const t = Object.defineProperty({}, "passive", { get: () => (e = true, e) });
        qe(document, x, Ge, t);
      } catch {
      }
      return e;
    })(), yn = ["webkitTransform", "transform"].some((e) => e in k.style), hn = "ontouchstart" in window || "msMaxTouchPoints" in navigator, An = ["webkitAnimation", "animation"].some((e) => e in k.style), wn = ["webkitTransition", "transition"].some((e) => e in k.style), Ke = (e, t) => e.getAttribute(t), kn = (e, t, n) => t.getAttributeNS(e, n), Sn = (e, t) => e.hasAttribute(t), Dn = (e, t, n) => t.hasAttributeNS(e, n), Mn = (e, t, n) => e.setAttribute(t, n), Nn = (e, t, n, o) => t.setAttributeNS(e, n, o), Tn = (e, t) => e.removeAttribute(t), Cn = (e, t, n) => t.removeAttributeNS(e, n), Ln = (e, ...t) => {
      e.classList.add(...t);
    }, On = (e, ...t) => {
      e.classList.remove(...t);
    }, In = (e, t) => e.classList.contains(t), { body: zn } = document, { documentElement: xn } = document, Hn = (e) => Array.from(e), E = (e) => e != null && typeof e == "object" || false, c = (e) => E(e) && typeof e.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some((t) => e.nodeType === t) || false, u = (e) => c(e) && e.nodeType === 1 || false, y = /* @__PURE__ */ new Map(), L = { set: (e, t, n) => {
      if (!u(e))
        return;
      y.has(t) || y.set(t, /* @__PURE__ */ new Map()), y.get(t).set(e, n);
    }, getAllFor: (e) => y.get(e) || null, get: (e, t) => {
      if (!u(e) || !t)
        return null;
      const n = L.getAllFor(t);
      return e && n && n.get(e) || null;
    }, remove: (e, t) => {
      const n = L.getAllFor(t);
      !n || !u(e) || (n.delete(e), n.size === 0 && y.delete(t));
    } }, Bn = (e, t) => L.get(e, t), S = (e) => typeof e == "string" || false, V = (e) => E(e) && e.constructor.name === "Window" || false, P = (e) => c(e) && e.nodeType === 9 || false, d = (e) => V(e) ? e.document : P(e) ? e : c(e) ? e.ownerDocument : window.document, D = (e, ...t) => Object.assign(e, ...t), Je = (e) => {
      if (!e)
        return;
      if (S(e))
        return d().createElement(e);
      const { tagName: t } = e, n = Je(t);
      if (!n)
        return;
      const o = { ...e };
      return delete o.tagName, D(n, o);
    }, Xe = (e, t) => {
      if (!e || !t)
        return;
      if (S(t))
        return d().createElementNS(e, t);
      const { tagName: n } = t, o = Xe(e, n);
      if (!o)
        return;
      const s = { ...t };
      return delete s.tagName, D(o, s);
    }, F = (e, t) => e.dispatchEvent(t), Vn = (e, t, n) => n.indexOf(e) === t, m = (e, t) => {
      const n = getComputedStyle(e), o = t.replace("webkit", "Webkit").replace(/([A-Z])/g, "-$1").toLowerCase();
      return n.getPropertyValue(o);
    }, Ye = (e) => {
      const t = m(e, H), n = m(e, Re), o = n.includes("ms") ? 1 : 1e3, s = t && t !== "none" ? parseFloat(n) * o : 0;
      return Number.isNaN(s) ? 0 : s;
    }, Ze = (e) => {
      const t = m(e, H), n = m(e, Fe), o = n.includes("ms") ? 1 : 1e3, s = t && t !== "none" ? parseFloat(n) * o : 0;
      return Number.isNaN(s) ? 0 : s;
    }, Pn = (e, t) => {
      let n = 0;
      const o = new Event(T), s = Ze(e), a = Ye(e);
      if (s) {
        const i = (l) => {
          l.target === e && (t.apply(e, [l]), e.removeEventListener(T, i), n = 1);
        };
        e.addEventListener(T, i), setTimeout(() => {
          n || F(e, o);
        }, s + a + 17);
      } else
        t.apply(e, [o]);
    }, $e = (e) => {
      const t = m(e, B), n = m(e, Ue), o = n.includes("ms") ? 1 : 1e3, s = t && t !== "none" ? parseFloat(n) * o : 0;
      return Number.isNaN(s) ? 0 : s;
    }, _e = (e) => {
      const t = m(e, B), n = m(e, We), o = n.includes("ms") ? 1 : 1e3, s = t && t !== "none" ? parseFloat(n) * o : 0;
      return Number.isNaN(s) ? 0 : s;
    }, Fn = (e, t) => {
      let n = 0;
      const o = new Event(C), s = _e(e), a = $e(e);
      if (s) {
        const i = (l) => {
          l.target === e && (t.apply(e, [l]), e.removeEventListener(C, i), n = 1);
        };
        e.addEventListener(C, i), setTimeout(() => {
          n || F(e, o);
        }, s + a + 17);
      } else
        t.apply(e, [o]);
    }, Rn = (e) => Float32Array.from(Array.from(e)), Wn = (e) => Float64Array.from(Array.from(e)), Un = (e, t) => e.focus(t), z = (e) => ["true", true].includes(e) ? true : ["false", false].includes(e) ? false : ["null", "", null, void 0].includes(e) ? null : e !== "" && !Number.isNaN(+e) ? +e : e, h = (e) => Object.entries(e), et = (e) => e.toLowerCase(), jn = (e, t, n, o) => {
      const s = { ...n }, a = { ...e.dataset }, i = { ...t }, l = {}, v = "title";
      return h(a).forEach(([r, p]) => {
        const N = o && typeof r == "string" && r.includes(o) ? r.replace(o, "").replace(/[A-Z]/g, (ut) => et(ut)) : r;
        l[N] = z(p);
      }), h(s).forEach(([r, p]) => {
        s[r] = z(p);
      }), h(t).forEach(([r, p]) => {
        r in s ? i[r] = s[r] : r in l ? i[r] = l[r] : i[r] = r === v ? Ke(e, v) : p;
      }), i;
    }, Qn = (e, t) => E(e) && (Object.hasOwn(e, t) || t in e), qn = (e) => Object.keys(e), Gn = (e) => Object.values(e), Kn = (e, t) => {
      const n = new CustomEvent(e, { cancelable: true, bubbles: true });
      return E(t) && D(n, t), n;
    }, Jn = { passive: true }, Xn = (e) => e.offsetHeight, Yn = (e, t) => {
      h(t).forEach(([n, o]) => {
        if (o && S(n) && n.includes("--"))
          e.style.setProperty(n, o);
        else {
          const s = {};
          s[n] = o, D(e.style, s);
        }
      });
    }, O = (e) => E(e) && e.constructor.name === "Map" || false, tt = (e) => typeof e == "number" || false, g = /* @__PURE__ */ new Map(), Zn = { set: (e, t, n, o) => {
      u(e) && (o && o.length ? (g.has(e) || g.set(e, /* @__PURE__ */ new Map()), g.get(e).set(o, setTimeout(t, n))) : g.set(e, setTimeout(t, n)));
    }, get: (e, t) => {
      if (!u(e))
        return null;
      const n = g.get(e);
      return t && n && O(n) ? n.get(t) || null : tt(n) ? n : null;
    }, clear: (e, t) => {
      if (!u(e))
        return;
      const n = g.get(e);
      t && t.length && O(n) ? (clearTimeout(n.get(t)), n.delete(t), n.size === 0 && g.delete(e)) : (clearTimeout(n), g.delete(e));
    } }, $n = (e) => e.toUpperCase(), b = (e, t) => {
      const { width: n, height: o, top: s, right: a, bottom: i, left: l } = e.getBoundingClientRect();
      let v = 1, r = 1;
      if (t && u(e)) {
        const { offsetWidth: p, offsetHeight: N } = e;
        v = p > 0 ? Math.round(n) / p : 1, r = N > 0 ? Math.round(o) / N : 1;
      }
      return { width: n / v, height: o / r, top: s / r, right: a / v, bottom: i / r, left: l / v, x: l / v, y: s / r };
    }, _n = (e) => d(e).body, M = (e) => d(e).documentElement, eo = (e) => d(e).head, to = (e) => {
      const t = V(e), n = t ? e.scrollX : e.scrollLeft, o = t ? e.scrollY : e.scrollTop;
      return { x: n, y: o };
    }, nt = (e) => c(e) && e.constructor.name === "ShadowRoot" || false, no = (e) => e.nodeName === "HTML" ? e : u(e) && e.assignedSlot || c(e) && e.parentNode || nt(e) && e.host || M(e), ot = (e) => {
      if (!u(e))
        return false;
      const { width: t, height: n } = b(e), { offsetWidth: o, offsetHeight: s } = e;
      return Math.round(t) !== o || Math.round(n) !== s;
    }, oo = (e, t, n) => {
      const o = u(t), s = b(e, o && ot(t)), a = { x: 0, y: 0 };
      if (o) {
        const i = b(t, true);
        a.x = i.x + t.clientLeft, a.y = i.y + t.clientTop;
      }
      return { x: s.left + n.x - a.x, y: s.top + n.y - a.y, width: s.width, height: s.height };
    };
    let U = 0, j = 0;
    const f = /* @__PURE__ */ new Map(), st = (e, t) => {
      let n = t ? U : j;
      if (t) {
        const o = st(e), s = f.get(o) || /* @__PURE__ */ new Map();
        f.has(o) || f.set(o, s), O(s) && !s.has(t) ? (s.set(t, n), U += 1) : n = s.get(t);
      } else {
        const o = e.id || e;
        f.has(o) ? n = f.get(o) : (f.set(o, n), j += 1);
      }
      return n;
    }, so = (e) => {
      var _a;
      return e ? P(e) ? e.defaultView : c(e) ? (_a = e == null ? void 0 : e.ownerDocument) == null ? void 0 : _a.defaultView : e : window;
    }, rt = (e) => Array.isArray(e) || false, ro = (e) => c(e) && e.nodeName === "CANVAS" || false, at = (e) => u(e) && !!e.shadowRoot || false, ao = (e) => c(e) && [1, 2, 3, 4, 5, 6, 7, 8].some((t) => e.nodeType === t) || false, co = (e) => {
      if (!c(e))
        return false;
      const { top: t, bottom: n } = b(e), { clientHeight: o } = M(e);
      return t <= o && n >= 0;
    }, io = (e) => {
      if (!c(e))
        return false;
      const { clientWidth: t, clientHeight: n } = M(e), { top: o, left: s, bottom: a, right: i } = b(e, true);
      return o >= 0 && s >= 0 && a <= n && i <= t;
    }, uo = (e) => rt(e) && e.every(u) || false, lo = (e) => typeof e == "function" || false, mo = (e) => E(e) && e.constructor.name === "HTMLCollection" || false, Eo = (e) => u(e) && e.tagName === "IMG" || false, vo = (e) => {
      if (!S(e))
        return false;
      try {
        JSON.parse(e);
      } catch {
        return false;
      }
      return true;
    }, po = (e) => E(e) && e.constructor.name === "WeakMap" || false, go = (e) => c(e) && ["SVG", "Image", "Video", "Canvas"].some((t) => e.constructor.name.includes(t)) || false, fo = (e) => E(e) && e.constructor.name === "NodeList" || false, bo = (e) => M(e).dir === "rtl", yo = (e) => c(e) && e.constructor.name.includes("SVG") || false, ho = (e) => c(e) && ["TABLE", "TD", "TH"].includes(e.nodeName) || false, ct = (e, t) => e ? e.closest(t) || ct(e.getRootNode().host, t) : null, Ao = (e, t) => u(e) ? e : (c(t) ? t : d()).querySelector(e), it = (e, t) => (c(t) ? t : d()).getElementsByTagName(e), wo = (e) => [...it("*", e)].filter(at), ko = (e, t) => d(t).getElementById(e) || null, So = (e, t) => (c(t) ? t : d()).querySelectorAll(e), Do = (e, t) => (t && c(t) ? t : d()).getElementsByClassName(e), Mo = (e, t) => e.matches(t), No = "2.0.0alpha12";
    exports2.ArrayFrom = Hn;
    exports2.DOMContentLoadedEvent = x;
    exports2.DOMMouseScrollEvent = X;
    exports2.Data = L;
    exports2.Float32ArrayFrom = Rn;
    exports2.Float64ArrayFrom = Wn;
    exports2.ObjectAssign = D;
    exports2.ObjectEntries = h;
    exports2.ObjectHasOwn = Qn;
    exports2.ObjectKeys = qn;
    exports2.ObjectValues = Gn;
    exports2.Timer = Zn;
    exports2.abortEvent = Q;
    exports2.addClass = Ln;
    exports2.addEventListener = on;
    exports2.animationDelay = Re;
    exports2.animationDuration = Fe;
    exports2.animationEndEvent = T;
    exports2.animationName = H;
    exports2.ariaChecked = lt;
    exports2.ariaDescribedBy = mt;
    exports2.ariaDescription = dt;
    exports2.ariaExpanded = Et;
    exports2.ariaHasPopup = vt;
    exports2.ariaHidden = pt;
    exports2.ariaLabel = gt;
    exports2.ariaLabelledBy = ft;
    exports2.ariaModal = bt;
    exports2.ariaPressed = yt;
    exports2.ariaSelected = ht;
    exports2.ariaValueMax = wt;
    exports2.ariaValueMin = At;
    exports2.ariaValueNow = kt;
    exports2.ariaValueText = St;
    exports2.beforeunloadEvent = q;
    exports2.bezierEasings = rn;
    exports2.blurEvent = G;
    exports2.changeEvent = K;
    exports2.closest = ct;
    exports2.contextmenuEvent = J;
    exports2.createCustomEvent = Kn;
    exports2.createElement = Je;
    exports2.createElementNS = Xe;
    exports2.dispatchEvent = F;
    exports2.distinct = Vn;
    exports2.documentBody = zn;
    exports2.documentElement = xn;
    exports2.documentHead = k;
    exports2.dragEvent = Mt;
    exports2.dragendEvent = Ot;
    exports2.dragenterEvent = Tt;
    exports2.dragleaveEvent = Ct;
    exports2.dragoverEvent = Lt;
    exports2.dragstartEvent = Nt;
    exports2.emulateAnimationEnd = Pn;
    exports2.emulateTransitionEnd = Fn;
    exports2.errorEvent = Y;
    exports2.focus = Un;
    exports2.focusEvent = Z;
    exports2.focusEvents = Vt;
    exports2.focusinEvent = $;
    exports2.focusoutEvent = _;
    exports2.gesturechangeEvent = ee;
    exports2.gestureendEvent = te;
    exports2.gesturestartEvent = ne;
    exports2.getAttribute = Ke;
    exports2.getAttributeNS = kn;
    exports2.getBoundingClientRect = b;
    exports2.getCustomElements = wo;
    exports2.getDocument = d;
    exports2.getDocumentBody = _n;
    exports2.getDocumentElement = M;
    exports2.getDocumentHead = eo;
    exports2.getElementAnimationDelay = Ye;
    exports2.getElementAnimationDuration = Ze;
    exports2.getElementById = ko;
    exports2.getElementStyle = m;
    exports2.getElementTransitionDelay = $e;
    exports2.getElementTransitionDuration = _e;
    exports2.getElementsByClassName = Do;
    exports2.getElementsByTagName = it;
    exports2.getInstance = Bn;
    exports2.getNodeScroll = to;
    exports2.getParentNode = no;
    exports2.getRectRelativeToOffsetParent = oo;
    exports2.getUID = st;
    exports2.getWindow = so;
    exports2.hasAttribute = Sn;
    exports2.hasAttributeNS = Dn;
    exports2.hasClass = In;
    exports2.isApple = pn;
    exports2.isArray = rt;
    exports2.isCanvas = ro;
    exports2.isCustomElement = at;
    exports2.isDocument = P;
    exports2.isElement = ao;
    exports2.isElementInScrollRange = co;
    exports2.isElementInViewport = io;
    exports2.isElementsArray = uo;
    exports2.isFirefox = gn;
    exports2.isFunction = lo;
    exports2.isHTMLCollection = mo;
    exports2.isHTMLElement = u;
    exports2.isHTMLImageElement = Eo;
    exports2.isJSON = vo;
    exports2.isMap = O;
    exports2.isMedia = go;
    exports2.isMobile = vn;
    exports2.isNode = c;
    exports2.isNodeList = fo;
    exports2.isNumber = tt;
    exports2.isObject = E;
    exports2.isRTL = bo;
    exports2.isSVGElement = yo;
    exports2.isScaledElement = ot;
    exports2.isShadowRoot = nt;
    exports2.isString = S;
    exports2.isTableElement = ho;
    exports2.isWeakMap = po;
    exports2.isWindow = V;
    exports2.keyAlt = Ft;
    exports2.keyArrowDown = Rt;
    exports2.keyArrowLeft = Ut;
    exports2.keyArrowRight = jt;
    exports2.keyArrowUp = Wt;
    exports2.keyBackspace = Qt;
    exports2.keyCapsLock = qt;
    exports2.keyControl = Gt;
    exports2.keyDelete = Kt;
    exports2.keyEnter = Jt;
    exports2.keyEscape = Xt;
    exports2.keyInsert = Yt;
    exports2.keyMeta = Zt;
    exports2.keyPause = $t;
    exports2.keyScrollLock = _t;
    exports2.keyShift = en;
    exports2.keySpace = tn;
    exports2.keyTab = nn;
    exports2.keyboardEventKeys = Pt;
    exports2.keydownEvent = oe;
    exports2.keypressEvent = se;
    exports2.keyupEvent = re;
    exports2.loadEvent = ae;
    exports2.loadstartEvent = It;
    exports2.matches = Mo;
    exports2.mouseClickEvents = xt;
    exports2.mouseHoverEvents = Ht;
    exports2.mouseSwipeEvents = zt;
    exports2.mouseclickEvent = ce;
    exports2.mousedblclickEvent = ie;
    exports2.mousedownEvent = ue;
    exports2.mouseenterEvent = me;
    exports2.mousehoverEvent = de;
    exports2.mouseinEvent = ve;
    exports2.mouseleaveEvent = Ee;
    exports2.mousemoveEvent = fe;
    exports2.mouseoutEvent = pe;
    exports2.mouseoverEvent = ge;
    exports2.mouseupEvent = le;
    exports2.mousewheelEvent = be;
    exports2.moveEvent = ye;
    exports2.nativeEvents = Dt;
    exports2.noop = Ge;
    exports2.normalizeOptions = jn;
    exports2.normalizeValue = z;
    exports2.off = Qe;
    exports2.offsetHeight = an;
    exports2.offsetWidth = cn;
    exports2.on = je;
    exports2.one = qe;
    exports2.orientationchangeEvent = he;
    exports2.passiveHandler = Jn;
    exports2.pointercancelEvent = Ae;
    exports2.pointerdownEvent = we;
    exports2.pointerleaveEvent = ke;
    exports2.pointermoveEvent = Se;
    exports2.pointerupEvent = De;
    exports2.querySelector = Ao;
    exports2.querySelectorAll = So;
    exports2.readystatechangeEvent = Me;
    exports2.reflow = Xn;
    exports2.removeAttribute = Tn;
    exports2.removeAttributeNS = Cn;
    exports2.removeClass = On;
    exports2.removeEventListener = sn;
    exports2.resetEvent = Ne;
    exports2.resizeEvent = Te;
    exports2.scrollEvent = Ie;
    exports2.scrollHeight = un;
    exports2.scrollWidth = ln;
    exports2.selectEvent = Ce;
    exports2.selectendEvent = Le;
    exports2.selectstartEvent = Oe;
    exports2.setAttribute = Mn;
    exports2.setAttributeNS = Nn;
    exports2.setElementStyle = Yn;
    exports2.submitEvent = ze;
    exports2.support3DTransform = fn;
    exports2.supportAnimation = An;
    exports2.supportPassive = bn;
    exports2.supportTouch = hn;
    exports2.supportTransform = yn;
    exports2.supportTransition = wn;
    exports2.tabindex = dn;
    exports2.toLowerCase = et;
    exports2.toUpperCase = $n;
    exports2.touchEvents = Bt;
    exports2.touchcancelEvent = Be;
    exports2.touchendEvent = Ve;
    exports2.touchmoveEvent = He;
    exports2.touchstartEvent = xe;
    exports2.transitionDelay = Ue;
    exports2.transitionDuration = We;
    exports2.transitionEndEvent = C;
    exports2.transitionProperty = B;
    exports2.unloadEvent = Pe;
    exports2.userAgent = w;
    exports2.userAgentData = A;
    exports2.version = No;
  })(shorty);
  const fadeClass = "fade";
  const showClass = "show";
  const dataBsDismiss = "data-bs-dismiss";
  const alertString = "alert";
  const alertComponent = "Alert";
  const version = "5.0.4";
  const Version = version;
  class BaseComponent {
    /**
     * @param target `HTMLElement` or selector string
     * @param config component instance options
     */
    constructor(target, config) {
      __publicField(this, "element");
      __publicField(this, "options");
      const element = shorty.querySelector(target);
      if (!element) {
        if (shorty.isString(target)) {
          throw Error(`${this.name} Error: "${target}" is not a valid selector.`);
        } else {
          throw Error(`${this.name} Error: your target is not an instance of HTMLElement.`);
        }
      }
      const prevInstance = shorty.Data.get(element, this.name);
      if (prevInstance)
        prevInstance.dispose();
      this.element = element;
      if (this.defaults && shorty.ObjectKeys(this.defaults).length) {
        this.options = shorty.normalizeOptions(element, this.defaults, config || {}, "bs");
      }
      shorty.Data.set(element, this.name, this);
    }
    /* istanbul ignore next */
    get version() {
      return Version;
    }
    /* istanbul ignore next */
    get name() {
      return "BaseComponent";
    }
    /* istanbul ignore next */
    get defaults() {
      return {};
    }
    /**
     * Removes component from target element;
     */
    dispose() {
      shorty.Data.remove(this.element, this.name);
      shorty.ObjectKeys(this).forEach((prop) => {
        delete this[prop];
      });
    }
  }
  const alertSelector = `.${alertString}`;
  const alertDismissSelector = `[${dataBsDismiss}="${alertString}"]`;
  const getAlertInstance = (element) => shorty.getInstance(element, alertComponent);
  const alertInitCallback = (element) => new Alert(element);
  const closeAlertEvent = shorty.createCustomEvent(`close.bs.${alertString}`);
  const closedAlertEvent = shorty.createCustomEvent(`closed.bs.${alertString}`);
  const alertTransitionEnd = (that) => {
    const { element } = that;
    toggleAlertHandler(that);
    shorty.dispatchEvent(element, closedAlertEvent);
    that.dispose();
    element.remove();
  };
  const toggleAlertHandler = (that, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { dismiss, close } = that;
    if (dismiss)
      action(dismiss, shorty.mouseclickEvent, close);
  };
  class Alert extends BaseComponent {
    constructor(target) {
      super(target);
      __publicField(this, "dismiss");
      // ALERT PUBLIC METHODS
      // ====================
      /**
       * Public method that hides the `.alert` element from the user,
       * disposes the instance once animation is complete, then
       * removes the element from the DOM.
       */
      __publicField(this, "close", () => {
        const { element } = this;
        if (element && shorty.hasClass(element, showClass)) {
          shorty.dispatchEvent(element, closeAlertEvent);
          if (closeAlertEvent.defaultPrevented)
            return;
          shorty.removeClass(element, showClass);
          if (shorty.hasClass(element, fadeClass)) {
            shorty.emulateTransitionEnd(element, () => alertTransitionEnd(this));
          } else
            alertTransitionEnd(this);
        }
      });
      this.dismiss = shorty.querySelector(alertDismissSelector, this.element);
      toggleAlertHandler(this, true);
    }
    /** Returns component name string. */
    get name() {
      return alertComponent;
    }
    /** Remove the component from target element. */
    dispose() {
      toggleAlertHandler(this);
      super.dispose();
    }
  }
  __publicField(Alert, "selector", alertSelector);
  __publicField(Alert, "init", alertInitCallback);
  __publicField(Alert, "getInstance", getAlertInstance);
  const activeClass = "active";
  const dataBsToggle = "data-bs-toggle";
  const buttonString = "button";
  const buttonComponent = "Button";
  const buttonSelector = `[${dataBsToggle}="${buttonString}"]`;
  const getButtonInstance = (element) => shorty.getInstance(element, buttonComponent);
  const buttonInitCallback = (element) => new Button(element);
  const toggleButtonHandler = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    action(self.element, shorty.mouseclickEvent, self.toggle);
  };
  class Button extends BaseComponent {
    /**
     * @param target usually a `.btn` element
     */
    constructor(target) {
      super(target);
      __publicField(this, "isActive", false);
      // BUTTON PUBLIC METHODS
      // =====================
      /**
       * Toggles the state of the target button.
       *
       * @param e usually `click` Event object
       */
      __publicField(this, "toggle", (e) => {
        if (e)
          e.preventDefault();
        const { element, isActive } = this;
        if (shorty.hasClass(element, "disabled"))
          return;
        const action = isActive ? shorty.removeClass : shorty.addClass;
        action(element, activeClass);
        shorty.setAttribute(element, shorty.ariaPressed, isActive ? "false" : "true");
        this.isActive = shorty.hasClass(element, activeClass);
      });
      const { element } = this;
      this.isActive = shorty.hasClass(element, activeClass);
      shorty.setAttribute(element, shorty.ariaPressed, String(!!this.isActive));
      toggleButtonHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return buttonComponent;
    }
    /** Removes the `Button` component from the target element. */
    dispose() {
      toggleButtonHandler(this);
      super.dispose();
    }
  }
  __publicField(Button, "selector", buttonSelector);
  __publicField(Button, "init", buttonInitCallback);
  __publicField(Button, "getInstance", getButtonInstance);
  const dataBsTarget = "data-bs-target";
  const carouselString = "carousel";
  const carouselComponent = "Carousel";
  const dataBsParent = "data-bs-parent";
  const dataBsContainer = "data-bs-container";
  const getTargetElement = (element) => {
    const targetAttr = [dataBsTarget, dataBsParent, dataBsContainer, "href"];
    const doc = shorty.getDocument(element);
    return targetAttr.map((att) => {
      const attValue = shorty.getAttribute(element, att);
      if (attValue) {
        return att === dataBsParent ? shorty.closest(element, attValue) : shorty.querySelector(attValue, doc);
      }
      return null;
    }).filter((x) => x)[0];
  };
  const carouselSelector = `[data-bs-ride="${carouselString}"]`;
  const carouselItem = `${carouselString}-item`;
  const dataBsSlideTo = "data-bs-slide-to";
  const dataBsSlide = "data-bs-slide";
  const pausedClass = "paused";
  const carouselDefaults = {
    pause: "hover",
    keyboard: false,
    touch: true,
    interval: 5e3
  };
  const getCarouselInstance = (element) => shorty.getInstance(element, carouselComponent);
  const carouselInitCallback = (element) => new Carousel(element);
  let startX = 0;
  let currentX = 0;
  let endX = 0;
  const carouselSlideEvent = shorty.createCustomEvent(`slide.bs.${carouselString}`);
  const carouselSlidEvent = shorty.createCustomEvent(`slid.bs.${carouselString}`);
  const carouselTransitionEndHandler = (self) => {
    const { index, direction, element, slides, options } = self;
    if (self.isAnimating && getCarouselInstance(element)) {
      const activeItem = getActiveIndex(self);
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      shorty.addClass(slides[index], activeClass);
      shorty.removeClass(slides[index], `${carouselItem}-${orientation}`);
      shorty.removeClass(slides[index], `${carouselItem}-${directionClass}`);
      shorty.removeClass(slides[activeItem], activeClass);
      shorty.removeClass(slides[activeItem], `${carouselItem}-${directionClass}`);
      shorty.dispatchEvent(element, carouselSlidEvent);
      shorty.Timer.clear(element, dataBsSlide);
      if (!shorty.getDocument(element).hidden && options.interval && !self.isPaused) {
        self.cycle();
      }
    }
  };
  function carouselPauseHandler() {
    const self = getCarouselInstance(this);
    if (self && !self.isPaused && !shorty.Timer.get(this, pausedClass)) {
      shorty.addClass(this, pausedClass);
    }
  }
  function carouselResumeHandler() {
    const self = getCarouselInstance(this);
    if (self && self.isPaused && !shorty.Timer.get(this, pausedClass)) {
      self.cycle();
    }
  }
  function carouselIndicatorHandler(e) {
    e.preventDefault();
    const element = shorty.closest(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating)
      return;
    const newIndex = +(shorty.getAttribute(this, dataBsSlideTo) || /* istanbul ignore next */
    0);
    if (this && !shorty.hasClass(this, activeClass) && // event target is not active
    !Number.isNaN(newIndex)) {
      self.to(newIndex);
    }
  }
  function carouselControlsHandler(e) {
    e.preventDefault();
    const element = shorty.closest(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating)
      return;
    const orientation = shorty.getAttribute(this, dataBsSlide);
    if (orientation === "next") {
      self.next();
    } else if (orientation === "prev") {
      self.prev();
    }
  }
  const carouselKeyHandler = ({ code, target }) => {
    const doc = shorty.getDocument(target);
    const [element] = [...shorty.querySelectorAll(carouselSelector, doc)].filter((x) => shorty.isElementInScrollRange(x));
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating || /textarea|input/i.test(target.nodeName))
      return;
    const RTL = shorty.isRTL(element);
    const arrowKeyNext = !RTL ? shorty.keyArrowRight : shorty.keyArrowLeft;
    const arrowKeyPrev = !RTL ? shorty.keyArrowLeft : shorty.keyArrowRight;
    if (code === arrowKeyPrev)
      self.prev();
    else if (code === arrowKeyNext)
      self.next();
  };
  function carouselDragHandler(e) {
    const { target } = e;
    const self = getCarouselInstance(this);
    if (self && self.isTouch && (self.indicator && !self.indicator.contains(target) || !self.controls.includes(target))) {
      e.stopImmediatePropagation();
      e.stopPropagation();
      e.preventDefault();
    }
  }
  function carouselPointerDownHandler(e) {
    const { target } = e;
    const self = getCarouselInstance(this);
    if (!self || self.isAnimating || self.isTouch) {
      return;
    }
    const { controls, indicators } = self;
    if ([...controls, ...indicators].some((el) => el === target || el.contains(target))) {
      return;
    }
    startX = e.pageX;
    if (this.contains(target)) {
      self.isTouch = true;
      toggleCarouselTouchHandlers(self, true);
    }
  }
  const carouselPointerMoveHandler = (e) => {
    currentX = e.pageX;
  };
  const carouselPointerUpHandler = (e) => {
    var _a;
    const { target } = e;
    const doc = shorty.getDocument(target);
    const self = [...shorty.querySelectorAll(carouselSelector, doc)].map((c) => getCarouselInstance(c)).find((i) => i.isTouch);
    if (!self) {
      return;
    }
    const { element, index } = self;
    const RTL = shorty.isRTL(element);
    self.isTouch = false;
    toggleCarouselTouchHandlers(self);
    if ((_a = doc.getSelection()) == null ? void 0 : _a.toString().length) {
      startX = 0;
      currentX = 0;
      endX = 0;
      return;
    }
    endX = e.pageX;
    if (!element.contains(target) || Math.abs(startX - endX) < 120) {
      startX = 0;
      currentX = 0;
      endX = 0;
      return;
    }
    if (currentX < startX) {
      self.to(index + (RTL ? -1 : 1));
    } else if (currentX > startX) {
      self.to(index + (RTL ? 1 : -1));
    }
    startX = 0;
    currentX = 0;
    endX = 0;
  };
  const activateCarouselIndicator = (self, pageIndex) => {
    const { indicators } = self;
    [...indicators].forEach((x) => shorty.removeClass(x, activeClass));
    if (self.indicators[pageIndex])
      shorty.addClass(indicators[pageIndex], activeClass);
  };
  const toggleCarouselTouchHandlers = (self, add) => {
    const { element } = self;
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    action(shorty.getDocument(element), shorty.pointermoveEvent, carouselPointerMoveHandler, shorty.passiveHandler);
    action(shorty.getDocument(element), shorty.pointerupEvent, carouselPointerUpHandler, shorty.passiveHandler);
  };
  const toggleCarouselHandlers = (self, add) => {
    const { element, options, slides, controls, indicators } = self;
    const { touch, pause, interval, keyboard } = options;
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    if (pause && interval) {
      action(element, shorty.mouseenterEvent, carouselPauseHandler);
      action(element, shorty.mouseleaveEvent, carouselResumeHandler);
    }
    if (touch && slides.length > 2) {
      action(element, shorty.pointerdownEvent, carouselPointerDownHandler, shorty.passiveHandler);
      action(element, shorty.touchstartEvent, carouselDragHandler, { passive: false });
      action(element, shorty.dragstartEvent, carouselDragHandler, { passive: false });
    }
    if (controls.length) {
      controls.forEach((arrow) => {
        if (arrow)
          action(arrow, shorty.mouseclickEvent, carouselControlsHandler);
      });
    }
    if (indicators.length) {
      indicators.forEach((indicator) => {
        action(indicator, shorty.mouseclickEvent, carouselIndicatorHandler);
      });
    }
    if (keyboard)
      action(shorty.getDocument(element), shorty.keydownEvent, carouselKeyHandler);
  };
  const getActiveIndex = (self) => {
    const { slides, element } = self;
    const activeItem = shorty.querySelector(`.${carouselItem}.${activeClass}`, element);
    return shorty.isHTMLElement(activeItem) ? [...slides].indexOf(activeItem) : -1;
  };
  class Carousel extends BaseComponent {
    /**
     * @param target mostly a `.carousel` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.direction = shorty.isRTL(element) ? "right" : "left";
      this.index = 0;
      this.isTouch = false;
      this.slides = shorty.getElementsByClassName(carouselItem, element);
      const { slides } = this;
      if (slides.length < 2) {
        return;
      }
      const doc = shorty.getDocument(element);
      this.controls = [
        ...shorty.querySelectorAll(`[${dataBsSlide}]`, element),
        ...shorty.querySelectorAll(`[${dataBsSlide}][${dataBsTarget}="#${element.id}"]`, doc)
      ];
      this.indicator = shorty.querySelector(`.${carouselString}-indicators`, element);
      this.indicators = [
        ...this.indicator ? shorty.querySelectorAll(`[${dataBsSlideTo}]`, this.indicator) : [],
        ...shorty.querySelectorAll(`[${dataBsSlideTo}][${dataBsTarget}="#${element.id}"]`, doc)
      ];
      const { options } = this;
      this.options.interval = options.interval === true ? carouselDefaults.interval : options.interval;
      if (getActiveIndex(this) < 0) {
        shorty.addClass(slides[0], activeClass);
        if (this.indicators.length)
          activateCarouselIndicator(this, 0);
      }
      toggleCarouselHandlers(this, true);
      if (options.interval)
        this.cycle();
    }
    /**
     * Returns component name string.
     */
    get name() {
      return carouselComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return carouselDefaults;
    }
    /**
     * Check if instance is paused.
     */
    get isPaused() {
      return shorty.hasClass(this.element, pausedClass);
    }
    /**
     * Check if instance is animating.
     */
    get isAnimating() {
      return shorty.querySelector(`.${carouselItem}-next,.${carouselItem}-prev`, this.element) !== null;
    }
    // CAROUSEL PUBLIC METHODS
    // =======================
    /** Slide automatically through items. */
    cycle() {
      const { element, options, isPaused, index } = this;
      shorty.Timer.clear(element, carouselString);
      if (isPaused) {
        shorty.Timer.clear(element, pausedClass);
        shorty.removeClass(element, pausedClass);
      }
      shorty.Timer.set(
        element,
        () => {
          if (this.element && !this.isPaused && !this.isTouch && shorty.isElementInScrollRange(element)) {
            this.to(index + 1);
          }
        },
        options.interval,
        carouselString
      );
    }
    /** Pause the automatic cycle. */
    pause() {
      const { element, options } = this;
      if (!this.isPaused && options.interval) {
        shorty.addClass(element, pausedClass);
        shorty.Timer.set(
          element,
          () => {
          },
          1,
          pausedClass
        );
      }
    }
    /** Slide to the next item. */
    next() {
      if (!this.isAnimating) {
        this.to(this.index + 1);
      }
    }
    /** Slide to the previous item. */
    prev() {
      if (!this.isAnimating) {
        this.to(this.index - 1);
      }
    }
    /**
     * Jump to the item with the `idx` index.
     *
     * @param idx the index of the item to jump to
     */
    to(idx) {
      const { element, slides, options } = this;
      const activeItem = getActiveIndex(this);
      const RTL = shorty.isRTL(element);
      let next = idx;
      if (this.isAnimating || activeItem === next || shorty.Timer.get(element, dataBsSlide))
        return;
      if (activeItem < next || activeItem === 0 && next === slides.length - 1) {
        this.direction = RTL ? "right" : "left";
      } else if (activeItem > next || activeItem === slides.length - 1 && next === 0) {
        this.direction = RTL ? "left" : "right";
      }
      const { direction } = this;
      if (next < 0) {
        next = slides.length - 1;
      } else if (next >= slides.length) {
        next = 0;
      }
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      const eventProperties = {
        relatedTarget: slides[next],
        from: activeItem,
        to: next,
        direction
      };
      shorty.ObjectAssign(carouselSlideEvent, eventProperties);
      shorty.ObjectAssign(carouselSlidEvent, eventProperties);
      shorty.dispatchEvent(element, carouselSlideEvent);
      if (carouselSlideEvent.defaultPrevented)
        return;
      this.index = next;
      activateCarouselIndicator(this, next);
      if (shorty.getElementTransitionDuration(slides[next]) && shorty.hasClass(element, "slide")) {
        shorty.Timer.set(
          element,
          () => {
            shorty.addClass(slides[next], `${carouselItem}-${orientation}`);
            shorty.reflow(slides[next]);
            shorty.addClass(slides[next], `${carouselItem}-${directionClass}`);
            shorty.addClass(slides[activeItem], `${carouselItem}-${directionClass}`);
            shorty.emulateTransitionEnd(slides[next], () => carouselTransitionEndHandler(this));
          },
          0,
          dataBsSlide
        );
      } else {
        shorty.addClass(slides[next], activeClass);
        shorty.removeClass(slides[activeItem], activeClass);
        shorty.Timer.set(
          element,
          () => {
            shorty.Timer.clear(element, dataBsSlide);
            if (element && options.interval && !this.isPaused) {
              this.cycle();
            }
            shorty.dispatchEvent(element, carouselSlidEvent);
          },
          0,
          dataBsSlide
        );
      }
    }
    /** Remove `Carousel` component from target. */
    dispose() {
      const { slides } = this;
      const itemClasses = ["start", "end", "prev", "next"];
      [...slides].forEach((slide, idx) => {
        if (shorty.hasClass(slide, activeClass))
          activateCarouselIndicator(this, idx);
        itemClasses.forEach((c) => shorty.removeClass(slide, `${carouselItem}-${c}`));
      });
      toggleCarouselHandlers(this);
      super.dispose();
    }
  }
  __publicField(Carousel, "selector", carouselSelector);
  __publicField(Carousel, "init", carouselInitCallback);
  __publicField(Carousel, "getInstance", getCarouselInstance);
  const collapsingClass = "collapsing";
  const collapseString = "collapse";
  const collapseComponent = "Collapse";
  const collapseSelector = `.${collapseString}`;
  const collapseToggleSelector = `[${dataBsToggle}="${collapseString}"]`;
  const collapseDefaults = { parent: null };
  const getCollapseInstance = (element) => shorty.getInstance(element, collapseComponent);
  const collapseInitCallback = (element) => new Collapse(element);
  const showCollapseEvent = shorty.createCustomEvent(`show.bs.${collapseString}`);
  const shownCollapseEvent = shorty.createCustomEvent(`shown.bs.${collapseString}`);
  const hideCollapseEvent = shorty.createCustomEvent(`hide.bs.${collapseString}`);
  const hiddenCollapseEvent = shorty.createCustomEvent(`hidden.bs.${collapseString}`);
  const expandCollapse = (self) => {
    const { element, parent, triggers } = self;
    shorty.dispatchEvent(element, showCollapseEvent);
    if (showCollapseEvent.defaultPrevented)
      return;
    shorty.Timer.set(element, shorty.noop, 17);
    if (parent)
      shorty.Timer.set(parent, shorty.noop, 17);
    shorty.addClass(element, collapsingClass);
    shorty.removeClass(element, collapseString);
    shorty.setElementStyle(element, { height: `${element.scrollHeight}px` });
    shorty.emulateTransitionEnd(element, () => {
      shorty.Timer.clear(element);
      if (parent)
        shorty.Timer.clear(parent);
      triggers.forEach((btn) => shorty.setAttribute(btn, shorty.ariaExpanded, "true"));
      shorty.removeClass(element, collapsingClass);
      shorty.addClass(element, collapseString);
      shorty.addClass(element, showClass);
      shorty.setElementStyle(element, { height: "" });
      shorty.dispatchEvent(element, shownCollapseEvent);
    });
  };
  const collapseContent = (self) => {
    const { element, parent, triggers } = self;
    shorty.dispatchEvent(element, hideCollapseEvent);
    if (hideCollapseEvent.defaultPrevented)
      return;
    shorty.Timer.set(element, shorty.noop, 17);
    if (parent)
      shorty.Timer.set(parent, shorty.noop, 17);
    shorty.setElementStyle(element, { height: `${element.scrollHeight}px` });
    shorty.removeClass(element, collapseString);
    shorty.removeClass(element, showClass);
    shorty.addClass(element, collapsingClass);
    shorty.reflow(element);
    shorty.setElementStyle(element, { height: "0px" });
    shorty.emulateTransitionEnd(element, () => {
      shorty.Timer.clear(element);
      if (parent)
        shorty.Timer.clear(parent);
      triggers.forEach((btn) => shorty.setAttribute(btn, shorty.ariaExpanded, "false"));
      shorty.removeClass(element, collapsingClass);
      shorty.addClass(element, collapseString);
      shorty.setElementStyle(element, { height: "" });
      shorty.dispatchEvent(element, hiddenCollapseEvent);
    });
  };
  const toggleCollapseHandler = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { triggers } = self;
    if (triggers.length) {
      triggers.forEach((btn) => action(btn, shorty.mouseclickEvent, collapseClickHandler));
    }
  };
  const collapseClickHandler = (e) => {
    const { target } = e;
    const trigger = target && shorty.closest(target, collapseToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getCollapseInstance(element);
    if (self)
      self.toggle();
    if (trigger && trigger.tagName === "A")
      e.preventDefault();
  };
  class Collapse extends BaseComponent {
    /**
     * @param target and `Element` that matches the selector
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      const doc = shorty.getDocument(element);
      this.triggers = [...shorty.querySelectorAll(collapseToggleSelector, doc)].filter((btn) => getTargetElement(btn) === element);
      this.parent = shorty.isHTMLElement(options.parent) ? options.parent : shorty.isString(options.parent) ? getTargetElement(element) || shorty.querySelector(options.parent, doc) : null;
      toggleCollapseHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return collapseComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return collapseDefaults;
    }
    // COLLAPSE PUBLIC METHODS
    // =======================
    /** Toggles the visibility of the collapse. */
    toggle() {
      if (!shorty.hasClass(this.element, showClass))
        this.show();
      else
        this.hide();
    }
    /** Hides the collapse. */
    hide() {
      const { triggers, element } = this;
      if (shorty.Timer.get(element))
        return;
      collapseContent(this);
      if (triggers.length) {
        triggers.forEach((btn) => shorty.addClass(btn, `${collapseString}d`));
      }
    }
    /** Shows the collapse. */
    show() {
      const { element, parent, triggers } = this;
      let activeCollapse;
      let activeCollapseInstance;
      if (parent) {
        activeCollapse = [...shorty.querySelectorAll(`.${collapseString}.${showClass}`, parent)].find(
          (i) => getCollapseInstance(i)
        );
        activeCollapseInstance = activeCollapse && getCollapseInstance(activeCollapse);
      }
      if ((!parent || !shorty.Timer.get(parent)) && !shorty.Timer.get(element)) {
        if (activeCollapseInstance && activeCollapse !== element) {
          collapseContent(activeCollapseInstance);
          activeCollapseInstance.triggers.forEach((btn) => {
            shorty.addClass(btn, `${collapseString}d`);
          });
        }
        expandCollapse(this);
        if (triggers.length) {
          triggers.forEach((btn) => shorty.removeClass(btn, `${collapseString}d`));
        }
      }
    }
    /** Remove the `Collapse` component from the target `Element`. */
    dispose() {
      toggleCollapseHandler(this);
      super.dispose();
    }
  }
  __publicField(Collapse, "selector", collapseSelector);
  __publicField(Collapse, "init", collapseInitCallback);
  __publicField(Collapse, "getInstance", getCollapseInstance);
  const dropdownMenuClasses = ["dropdown", "dropup", "dropstart", "dropend"];
  const dropdownComponent = "Dropdown";
  const dropdownMenuClass = "dropdown-menu";
  const isEmptyAnchor = (element) => {
    const parentAnchor = shorty.closest(element, "A");
    return element.tagName === "A" && // anchor href starts with #
    shorty.hasAttribute(element, "href") && element.href.slice(-1) === "#" || // OR a child of an anchor with href starts with #
    parentAnchor && shorty.hasAttribute(parentAnchor, "href") && parentAnchor.href.slice(-1) === "#";
  };
  const [dropdownString, dropupString, dropstartString, dropendString] = dropdownMenuClasses;
  const dropdownSelector = `[${dataBsToggle}="${dropdownString}"],[${dataBsToggle}="${dropupString}"],[${dataBsToggle}="${dropendString}"],[${dataBsToggle}="${dropstartString}"]`;
  const getDropdownInstance = (element) => shorty.getInstance(element, dropdownComponent);
  const dropdownInitCallback = (element) => new Dropdown(element);
  const dropdownMenuEndClass = `${dropdownMenuClass}-end`;
  const verticalClass = [dropdownString, dropupString];
  const horizontalClass = [dropstartString, dropendString];
  const menuFocusTags = ["A", "BUTTON"];
  const dropdownDefaults = {
    offset: 5,
    // [number] 5(px)
    display: "dynamic"
    // [dynamic|static]
  };
  const showDropdownEvent = shorty.createCustomEvent(`show.bs.${dropdownString}`);
  const shownDropdownEvent = shorty.createCustomEvent(`shown.bs.${dropdownString}`);
  const hideDropdownEvent = shorty.createCustomEvent(`hide.bs.${dropdownString}`);
  const hiddenDropdownEvent = shorty.createCustomEvent(`hidden.bs.${dropdownString}`);
  const updatedDropdownEvent = shorty.createCustomEvent(`updated.bs.${dropdownString}`);
  const styleDropdown = (self) => {
    const { element, menu, parentElement, options } = self;
    const { offset } = options;
    if (shorty.getElementStyle(menu, "position") === "static")
      return;
    const RTL = shorty.isRTL(element);
    const menuEnd = shorty.hasClass(menu, dropdownMenuEndClass);
    const resetProps = ["margin", "top", "bottom", "left", "right"];
    resetProps.forEach((p) => {
      const style = {};
      style[p] = "";
      shorty.setElementStyle(menu, style);
    });
    let positionClass = dropdownMenuClasses.find((c) => shorty.hasClass(parentElement, c)) || /* istanbul ignore next: fallback position */
    dropdownString;
    const dropdownMargin = {
      dropdown: [offset, 0, 0],
      dropup: [0, 0, offset],
      dropstart: RTL ? [-1, 0, 0, offset] : [-1, offset, 0],
      dropend: RTL ? [-1, offset, 0] : [-1, 0, 0, offset]
    };
    const dropdownPosition = {
      dropdown: { top: "100%" },
      dropup: { top: "auto", bottom: "100%" },
      dropstart: RTL ? { left: "100%", right: "auto" } : { left: "auto", right: "100%" },
      dropend: RTL ? { left: "auto", right: "100%" } : { left: "100%", right: "auto" },
      menuStart: RTL ? { right: "0", left: "auto" } : { right: "auto", left: "0" },
      menuEnd: RTL ? { right: "auto", left: "0" } : { right: "0", left: "auto" }
    };
    const { offsetWidth: menuWidth, offsetHeight: menuHeight } = menu;
    const { clientWidth, clientHeight } = shorty.getDocumentElement(element);
    const { left: targetLeft, top: targetTop, width: targetWidth, height: targetHeight } = shorty.getBoundingClientRect(element);
    const leftFullExceed = targetLeft - menuWidth - offset < 0;
    const rightFullExceed = targetLeft + menuWidth + targetWidth + offset >= clientWidth;
    const bottomExceed = targetTop + menuHeight + offset >= clientHeight;
    const bottomFullExceed = targetTop + menuHeight + targetHeight + offset >= clientHeight;
    const topExceed = targetTop - menuHeight - offset < 0;
    const leftExceed = (!RTL && menuEnd || RTL && !menuEnd) && targetLeft + targetWidth - menuWidth < 0;
    const rightExceed = (RTL && menuEnd || !RTL && !menuEnd) && targetLeft + menuWidth >= clientWidth;
    if (horizontalClass.includes(positionClass) && leftFullExceed && rightFullExceed) {
      positionClass = dropdownString;
    }
    if (positionClass === dropstartString && (!RTL ? leftFullExceed : rightFullExceed)) {
      positionClass = dropendString;
    }
    if (positionClass === dropendString && (RTL ? leftFullExceed : rightFullExceed)) {
      positionClass = dropstartString;
    }
    if (positionClass === dropupString && topExceed && !bottomFullExceed) {
      positionClass = dropdownString;
    }
    if (positionClass === dropdownString && bottomFullExceed && !topExceed) {
      positionClass = dropupString;
    }
    if (horizontalClass.includes(positionClass) && bottomExceed) {
      shorty.ObjectAssign(dropdownPosition[positionClass], {
        top: "auto",
        bottom: 0
      });
    }
    if (verticalClass.includes(positionClass) && (leftExceed || rightExceed)) {
      let posAjust = { left: "auto", right: "auto" };
      if (!leftExceed && rightExceed && !RTL)
        posAjust = { left: "auto", right: 0 };
      if (leftExceed && !rightExceed && RTL)
        posAjust = { left: 0, right: "auto" };
      if (posAjust)
        shorty.ObjectAssign(dropdownPosition[positionClass], posAjust);
    }
    const margins = dropdownMargin[positionClass];
    shorty.setElementStyle(menu, {
      ...dropdownPosition[positionClass],
      margin: `${margins.map((x) => x ? `${x}px` : x).join(" ")}`
    });
    if (verticalClass.includes(positionClass) && menuEnd) {
      if (menuEnd) {
        const endAdjust = !RTL && leftExceed || RTL && rightExceed ? "menuStart" : (
          /* istanbul ignore next */
          "menuEnd"
        );
        shorty.setElementStyle(menu, dropdownPosition[endAdjust]);
      }
    }
    shorty.dispatchEvent(parentElement, updatedDropdownEvent);
  };
  const getMenuItems = (menu) => {
    return [...menu.children].map((c) => {
      if (c && menuFocusTags.includes(c.tagName))
        return c;
      const { firstElementChild } = c;
      if (firstElementChild && menuFocusTags.includes(firstElementChild.tagName)) {
        return firstElementChild;
      }
      return null;
    }).filter((c) => c);
  };
  const toggleDropdownDismiss = (self) => {
    const { element, options } = self;
    const action = self.open ? eventListener$1.addListener : eventListener$1.removeListener;
    const doc = shorty.getDocument(element);
    action(doc, shorty.mouseclickEvent, dropdownDismissHandler);
    action(doc, shorty.focusEvent, dropdownDismissHandler);
    action(doc, shorty.keydownEvent, dropdownPreventScroll);
    action(doc, shorty.keyupEvent, dropdownKeyHandler);
    if (options.display === "dynamic") {
      [shorty.scrollEvent, shorty.resizeEvent].forEach((ev) => {
        action(shorty.getWindow(element), ev, dropdownLayoutHandler, shorty.passiveHandler);
      });
    }
  };
  const toggleDropdownHandler = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    action(self.element, shorty.mouseclickEvent, dropdownClickHandler);
  };
  const getCurrentOpenDropdown = (element) => {
    const currentParent = [...dropdownMenuClasses, "btn-group", "input-group"].map((c) => shorty.getElementsByClassName(`${c} ${showClass}`, shorty.getDocument(element))).find((x) => x.length);
    if (currentParent && currentParent.length) {
      return [...currentParent[0].children].find(
        (x) => dropdownMenuClasses.some((c) => c === shorty.getAttribute(x, dataBsToggle))
      );
    }
    return void 0;
  };
  const dropdownDismissHandler = (e) => {
    const { target, type } = e;
    if (!target || !target.closest)
      return;
    const element = getCurrentOpenDropdown(target);
    const self = element && getDropdownInstance(element);
    if (!self)
      return;
    const { parentElement, menu } = self;
    const hasData = shorty.closest(target, dropdownSelector) !== null;
    const isForm = parentElement && parentElement.contains(target) && (target.tagName === "form" || shorty.closest(target, "form") !== null);
    if (type === shorty.mouseclickEvent && isEmptyAnchor(target)) {
      e.preventDefault();
    }
    if (type === shorty.focusEvent && (target === element || target === menu || menu.contains(target))) {
      return;
    }
    if (isForm || hasData)
      ;
    else if (self) {
      self.hide();
    }
  };
  const dropdownClickHandler = (e) => {
    const { target } = e;
    const element = target && shorty.closest(target, dropdownSelector);
    const self = element && getDropdownInstance(element);
    if (self) {
      e.stopImmediatePropagation();
      self.toggle();
      if (element && isEmptyAnchor(element))
        e.preventDefault();
    }
  };
  const dropdownPreventScroll = (e) => {
    if ([shorty.keyArrowDown, shorty.keyArrowUp].includes(e.code))
      e.preventDefault();
  };
  function dropdownKeyHandler(e) {
    const { code } = e;
    const element = getCurrentOpenDropdown(this);
    const self = element && getDropdownInstance(element);
    const { activeElement } = element && shorty.getDocument(element);
    if (!self || !activeElement)
      return;
    const { menu, open } = self;
    const menuItems = getMenuItems(menu);
    if (menuItems && menuItems.length && [shorty.keyArrowDown, shorty.keyArrowUp].includes(code)) {
      let idx = menuItems.indexOf(activeElement);
      if (activeElement === element) {
        idx = 0;
      } else if (code === shorty.keyArrowUp) {
        idx = idx > 1 ? idx - 1 : 0;
      } else if (code === shorty.keyArrowDown) {
        idx = idx < menuItems.length - 1 ? idx + 1 : idx;
      }
      if (menuItems[idx])
        shorty.focus(menuItems[idx]);
    }
    if (shorty.keyEscape === code && open) {
      self.toggle();
      shorty.focus(element);
    }
  }
  function dropdownLayoutHandler() {
    const element = getCurrentOpenDropdown(this);
    const self = element && getDropdownInstance(element);
    if (self && self.open)
      styleDropdown(self);
  }
  class Dropdown extends BaseComponent {
    /**
     * @param target Element or string selector
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { parentElement } = this.element;
      const menu = shorty.querySelector(`.${dropdownMenuClass}`, parentElement);
      if (!menu)
        return;
      this.parentElement = parentElement;
      this.menu = menu;
      toggleDropdownHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return dropdownComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return dropdownDefaults;
    }
    // DROPDOWN PUBLIC METHODS
    // =======================
    /** Shows/hides the dropdown menu to the user. */
    toggle() {
      if (this.open)
        this.hide();
      else
        this.show();
    }
    /** Shows the dropdown menu to the user. */
    show() {
      const { element, open, menu, parentElement } = this;
      if (open)
        return;
      const currentElement = getCurrentOpenDropdown(element);
      const currentInstance = currentElement && getDropdownInstance(currentElement);
      if (currentInstance)
        currentInstance.hide();
      [showDropdownEvent, shownDropdownEvent, updatedDropdownEvent].forEach((e) => {
        e.relatedTarget = element;
      });
      shorty.dispatchEvent(parentElement, showDropdownEvent);
      if (showDropdownEvent.defaultPrevented)
        return;
      shorty.addClass(menu, showClass);
      shorty.addClass(parentElement, showClass);
      shorty.setAttribute(element, shorty.ariaExpanded, "true");
      styleDropdown(this);
      this.open = !open;
      shorty.focus(element);
      toggleDropdownDismiss(this);
      shorty.dispatchEvent(parentElement, shownDropdownEvent);
    }
    /** Hides the dropdown menu from the user. */
    hide() {
      const { element, open, menu, parentElement } = this;
      if (!open)
        return;
      [hideDropdownEvent, hiddenDropdownEvent].forEach((e) => {
        e.relatedTarget = element;
      });
      shorty.dispatchEvent(parentElement, hideDropdownEvent);
      if (hideDropdownEvent.defaultPrevented)
        return;
      shorty.removeClass(menu, showClass);
      shorty.removeClass(parentElement, showClass);
      shorty.setAttribute(element, shorty.ariaExpanded, "false");
      this.open = !open;
      toggleDropdownDismiss(this);
      shorty.dispatchEvent(parentElement, hiddenDropdownEvent);
    }
    /** Removes the `Dropdown` component from the target element. */
    dispose() {
      if (this.open)
        this.hide();
      toggleDropdownHandler(this);
      super.dispose();
    }
  }
  __publicField(Dropdown, "selector", dropdownSelector);
  __publicField(Dropdown, "init", dropdownInitCallback);
  __publicField(Dropdown, "getInstance", getDropdownInstance);
  const modalString = "modal";
  const modalComponent = "Modal";
  const offcanvasComponent = "Offcanvas";
  const fixedTopClass = "fixed-top";
  const fixedBottomClass = "fixed-bottom";
  const stickyTopClass = "sticky-top";
  const positionStickyClass = "position-sticky";
  const getFixedItems = (parent) => [
    ...shorty.getElementsByClassName(fixedTopClass, parent),
    ...shorty.getElementsByClassName(fixedBottomClass, parent),
    ...shorty.getElementsByClassName(stickyTopClass, parent),
    ...shorty.getElementsByClassName(positionStickyClass, parent),
    ...shorty.getElementsByClassName("is-fixed", parent)
  ];
  const resetScrollbar = (element) => {
    const bd = shorty.getDocumentBody(element);
    shorty.setElementStyle(bd, {
      paddingRight: "",
      overflow: ""
    });
    const fixedItems = getFixedItems(bd);
    if (fixedItems.length) {
      fixedItems.forEach((fixed) => {
        shorty.setElementStyle(fixed, {
          paddingRight: "",
          marginRight: ""
        });
      });
    }
  };
  const measureScrollbar = (element) => {
    const { clientWidth } = shorty.getDocumentElement(element);
    const { innerWidth } = shorty.getWindow(element);
    return Math.abs(innerWidth - clientWidth);
  };
  const setScrollbar = (element, overflow) => {
    const bd = shorty.getDocumentBody(element);
    const bodyPad = parseInt(shorty.getElementStyle(bd, "paddingRight"), 10);
    const isOpen = shorty.getElementStyle(bd, "overflow") === "hidden";
    const sbWidth = isOpen && bodyPad ? 0 : measureScrollbar(element);
    const fixedItems = getFixedItems(bd);
    if (overflow) {
      shorty.setElementStyle(bd, {
        overflow: "hidden",
        paddingRight: `${bodyPad + sbWidth}px`
      });
      if (fixedItems.length) {
        fixedItems.forEach((fixed) => {
          const itemPadValue = shorty.getElementStyle(fixed, "paddingRight");
          fixed.style.paddingRight = `${parseInt(itemPadValue, 10) + sbWidth}px`;
          if ([stickyTopClass, positionStickyClass].some((c) => shorty.hasClass(fixed, c))) {
            const itemMValue = shorty.getElementStyle(fixed, "marginRight");
            fixed.style.marginRight = `${parseInt(itemMValue, 10) - sbWidth}px`;
          }
        });
      }
    }
  };
  const offcanvasString = "offcanvas";
  const popupContainer = shorty.createElement({ tagName: "div", className: "popup-container" });
  const appendPopup = (target, customContainer) => {
    const containerIsBody = shorty.isNode(customContainer) && customContainer.nodeName === "BODY";
    const lookup = shorty.isNode(customContainer) && !containerIsBody ? customContainer : popupContainer;
    const BODY = containerIsBody ? customContainer : shorty.getDocumentBody(target);
    if (shorty.isNode(target)) {
      if (lookup === popupContainer) {
        BODY.append(popupContainer);
      }
      lookup.append(target);
    }
  };
  const removePopup = (target, customContainer) => {
    const containerIsBody = shorty.isNode(customContainer) && customContainer.nodeName === "BODY";
    const lookup = shorty.isNode(customContainer) && !containerIsBody ? customContainer : popupContainer;
    if (shorty.isNode(target)) {
      target.remove();
      if (lookup === popupContainer && !popupContainer.children.length) {
        popupContainer.remove();
      }
    }
  };
  const hasPopup = (target, customContainer) => {
    const lookup = shorty.isNode(customContainer) && customContainer.nodeName !== "BODY" ? customContainer : popupContainer;
    return shorty.isNode(target) && lookup.contains(target);
  };
  const backdropString = "backdrop";
  const modalBackdropClass = `${modalString}-${backdropString}`;
  const offcanvasBackdropClass = `${offcanvasString}-${backdropString}`;
  const modalActiveSelector = `.${modalString}.${showClass}`;
  const offcanvasActiveSelector = `.${offcanvasString}.${showClass}`;
  const overlay = shorty.createElement("div");
  const getCurrentOpen = (element) => {
    return shorty.querySelector(`${modalActiveSelector},${offcanvasActiveSelector}`, shorty.getDocument(element));
  };
  const toggleOverlayType = (isModal) => {
    const targetClass = isModal ? modalBackdropClass : offcanvasBackdropClass;
    [modalBackdropClass, offcanvasBackdropClass].forEach((c) => {
      shorty.removeClass(overlay, c);
    });
    shorty.addClass(overlay, targetClass);
  };
  const appendOverlay = (element, hasFade, isModal) => {
    toggleOverlayType(isModal);
    appendPopup(overlay, shorty.getDocumentBody(element));
    if (hasFade)
      shorty.addClass(overlay, fadeClass);
  };
  const showOverlay = () => {
    if (!shorty.hasClass(overlay, showClass)) {
      shorty.addClass(overlay, showClass);
      shorty.reflow(overlay);
    }
  };
  const hideOverlay = () => {
    shorty.removeClass(overlay, showClass);
  };
  const removeOverlay = (element) => {
    if (!getCurrentOpen(element)) {
      shorty.removeClass(overlay, fadeClass);
      removePopup(overlay, shorty.getDocumentBody(element));
      resetScrollbar(element);
    }
  };
  const isVisible = (element) => {
    return shorty.isHTMLElement(element) && shorty.getElementStyle(element, "visibility") !== "hidden" && element.offsetParent !== null;
  };
  const modalSelector = `.${modalString}`;
  const modalToggleSelector = `[${dataBsToggle}="${modalString}"]`;
  const modalDismissSelector = `[${dataBsDismiss}="${modalString}"]`;
  const modalStaticClass = `${modalString}-static`;
  const modalDefaults = {
    backdrop: true,
    keyboard: true
  };
  const getModalInstance = (element) => shorty.getInstance(element, modalComponent);
  const modalInitCallback = (element) => new Modal(element);
  const showModalEvent = shorty.createCustomEvent(`show.bs.${modalString}`);
  const shownModalEvent = shorty.createCustomEvent(`shown.bs.${modalString}`);
  const hideModalEvent = shorty.createCustomEvent(`hide.bs.${modalString}`);
  const hiddenModalEvent = shorty.createCustomEvent(`hidden.bs.${modalString}`);
  const setModalScrollbar = (self) => {
    const { element } = self;
    const scrollbarWidth = measureScrollbar(element);
    const { clientHeight, scrollHeight } = shorty.getDocumentElement(element);
    const { clientHeight: modalHeight, scrollHeight: modalScrollHeight } = element;
    const modalOverflow = modalHeight !== modalScrollHeight;
    if (!modalOverflow && scrollbarWidth) {
      const pad = !shorty.isRTL(element) ? "paddingRight" : (
        /* istanbul ignore next */
        "paddingLeft"
      );
      const padStyle = {};
      padStyle[pad] = `${scrollbarWidth}px`;
      shorty.setElementStyle(element, padStyle);
    }
    setScrollbar(element, modalOverflow || clientHeight !== scrollHeight);
  };
  const toggleModalDismiss = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { element, update } = self;
    action(element, shorty.mouseclickEvent, modalDismissHandler);
    action(shorty.getWindow(element), shorty.resizeEvent, update, shorty.passiveHandler);
    action(shorty.getDocument(element), shorty.keydownEvent, modalKeyHandler);
  };
  const toggleModalHandler = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { triggers } = self;
    if (triggers.length) {
      triggers.forEach((btn) => action(btn, shorty.mouseclickEvent, modalClickHandler));
    }
  };
  const afterModalHide = (self, callback) => {
    const { triggers, element, relatedTarget } = self;
    removeOverlay(element);
    shorty.setElementStyle(element, { paddingRight: "", display: "" });
    toggleModalDismiss(self);
    const focusElement = showModalEvent.relatedTarget || triggers.find(isVisible);
    if (focusElement)
      shorty.focus(focusElement);
    if (shorty.isFunction(callback))
      callback();
    hiddenModalEvent.relatedTarget = relatedTarget;
    shorty.dispatchEvent(element, hiddenModalEvent);
  };
  const afterModalShow = (self) => {
    const { element, relatedTarget } = self;
    shorty.focus(element);
    toggleModalDismiss(self, true);
    shownModalEvent.relatedTarget = relatedTarget;
    shorty.dispatchEvent(element, shownModalEvent);
  };
  const beforeModalShow = (self) => {
    const { element, hasFade } = self;
    shorty.setElementStyle(element, { display: "block" });
    setModalScrollbar(self);
    if (!getCurrentOpen(element)) {
      shorty.setElementStyle(shorty.getDocumentBody(element), { overflow: "hidden" });
    }
    shorty.addClass(element, showClass);
    shorty.removeAttribute(element, shorty.ariaHidden);
    shorty.setAttribute(element, shorty.ariaModal, "true");
    if (hasFade)
      shorty.emulateTransitionEnd(element, () => afterModalShow(self));
    else
      afterModalShow(self);
  };
  const beforeModalHide = (self, callback) => {
    const { element, options, hasFade } = self;
    if (options.backdrop && !callback && hasFade && shorty.hasClass(overlay, showClass) && !getCurrentOpen(element)) {
      hideOverlay();
      shorty.emulateTransitionEnd(overlay, () => afterModalHide(self));
    } else {
      afterModalHide(self, callback);
    }
  };
  const modalClickHandler = (e) => {
    const { target } = e;
    const trigger = target && shorty.closest(target, modalToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getModalInstance(element);
    if (!self)
      return;
    if (trigger && trigger.tagName === "A")
      e.preventDefault();
    self.relatedTarget = trigger;
    self.toggle();
  };
  const modalKeyHandler = ({ code, target }) => {
    const element = shorty.querySelector(modalActiveSelector, shorty.getDocument(target));
    const self = element && getModalInstance(element);
    if (!self)
      return;
    const { options } = self;
    if (options.keyboard && code === shorty.keyEscape && // the keyboard option is enabled and the key is 27
    shorty.hasClass(element, showClass)) {
      self.relatedTarget = null;
      self.hide();
    }
  };
  function modalDismissHandler(e) {
    var _a, _b;
    const self = getModalInstance(this);
    if (!self || shorty.Timer.get(this))
      return;
    const { options, isStatic, modalDialog } = self;
    const { backdrop } = options;
    const { target } = e;
    const selectedText = (_b = (_a = shorty.getDocument(this)) == null ? void 0 : _a.getSelection()) == null ? void 0 : _b.toString().length;
    const targetInsideDialog = modalDialog == null ? void 0 : modalDialog.contains(target);
    const dismiss = target && shorty.closest(target, modalDismissSelector);
    if (isStatic && !targetInsideDialog) {
      shorty.Timer.set(
        this,
        () => {
          shorty.addClass(this, modalStaticClass);
          shorty.emulateTransitionEnd(modalDialog, () => staticTransitionEnd(self));
        },
        17
      );
    } else if (dismiss || !selectedText && !isStatic && !targetInsideDialog && backdrop) {
      self.relatedTarget = dismiss || null;
      self.hide();
      e.preventDefault();
    }
  }
  const staticTransitionEnd = (self) => {
    const { element, modalDialog } = self;
    const duration = (shorty.isHTMLElement(modalDialog) ? shorty.getElementTransitionDuration(modalDialog) : (
      /* istanbul ignore next */
      0
    )) + 17;
    shorty.removeClass(element, modalStaticClass);
    shorty.Timer.set(element, () => shorty.Timer.clear(element), duration);
  };
  class Modal extends BaseComponent {
    /**
     * @param target usually the `.modal` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      /**
       * Updates the modal layout.
       */
      __publicField(this, "update", () => {
        if (shorty.hasClass(this.element, showClass))
          setModalScrollbar(this);
      });
      const { element } = this;
      this.modalDialog = shorty.querySelector(`.${modalString}-dialog`, element);
      this.triggers = [...shorty.querySelectorAll(modalToggleSelector, shorty.getDocument(element))].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.isStatic = this.options.backdrop === "static";
      this.hasFade = shorty.hasClass(element, fadeClass);
      this.relatedTarget = null;
      toggleModalHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return modalComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return modalDefaults;
    }
    // MODAL PUBLIC METHODS
    // ====================
    /** Toggles the visibility of the modal. */
    toggle() {
      if (shorty.hasClass(this.element, showClass))
        this.hide();
      else
        this.show();
    }
    /** Shows the modal to the user. */
    show() {
      const { element, options, hasFade, relatedTarget } = this;
      const { backdrop } = options;
      let overlayDelay = 0;
      if (shorty.hasClass(element, showClass))
        return;
      showModalEvent.relatedTarget = relatedTarget || void 0;
      shorty.dispatchEvent(element, showModalEvent);
      if (showModalEvent.defaultPrevented)
        return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getModalInstance(currentOpen) || /* istanbul ignore next */
        shorty.getInstance(currentOpen, offcanvasComponent);
        if (that)
          that.hide();
      }
      if (backdrop) {
        if (!hasPopup(overlay)) {
          appendOverlay(element, hasFade, true);
        } else {
          toggleOverlayType(true);
        }
        overlayDelay = shorty.getElementTransitionDuration(overlay);
        showOverlay();
        setTimeout(() => beforeModalShow(this), overlayDelay);
      } else {
        beforeModalShow(this);
        if (currentOpen && shorty.hasClass(overlay, showClass)) {
          hideOverlay();
        }
      }
    }
    /**
     * Hide the modal from the user.
     *
     * @param callback when defined it will skip animation
     */
    hide(callback) {
      const { element, hasFade, relatedTarget } = this;
      if (!shorty.hasClass(element, showClass))
        return;
      hideModalEvent.relatedTarget = relatedTarget || void 0;
      shorty.dispatchEvent(element, hideModalEvent);
      if (hideModalEvent.defaultPrevented)
        return;
      shorty.removeClass(element, showClass);
      shorty.setAttribute(element, shorty.ariaHidden, "true");
      shorty.removeAttribute(element, shorty.ariaModal);
      if (hasFade) {
        shorty.emulateTransitionEnd(element, () => beforeModalHide(this, callback));
      } else {
        beforeModalHide(this, callback);
      }
    }
    /** Removes the `Modal` component from target element. */
    dispose() {
      toggleModalHandler(this);
      this.hide(() => super.dispose());
    }
  }
  __publicField(Modal, "selector", modalSelector);
  __publicField(Modal, "init", modalInitCallback);
  __publicField(Modal, "getInstance", getModalInstance);
  const offcanvasSelector = `.${offcanvasString}`;
  const offcanvasToggleSelector = `[${dataBsToggle}="${offcanvasString}"]`;
  const offcanvasDismissSelector = `[${dataBsDismiss}="${offcanvasString}"]`;
  const offcanvasTogglingClass = `${offcanvasString}-toggling`;
  const offcanvasDefaults = {
    backdrop: true,
    // boolean
    keyboard: true,
    // boolean
    scroll: false
    // boolean
  };
  const getOffcanvasInstance = (element) => shorty.getInstance(element, offcanvasComponent);
  const offcanvasInitCallback = (element) => new Offcanvas(element);
  const showOffcanvasEvent = shorty.createCustomEvent(`show.bs.${offcanvasString}`);
  const shownOffcanvasEvent = shorty.createCustomEvent(`shown.bs.${offcanvasString}`);
  const hideOffcanvasEvent = shorty.createCustomEvent(`hide.bs.${offcanvasString}`);
  const hiddenOffcanvasEvent = shorty.createCustomEvent(`hidden.bs.${offcanvasString}`);
  const setOffCanvasScrollbar = (self) => {
    const { element } = self;
    const { clientHeight, scrollHeight } = shorty.getDocumentElement(element);
    setScrollbar(element, clientHeight !== scrollHeight);
  };
  const toggleOffcanvasEvents = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    self.triggers.forEach((btn) => action(btn, shorty.mouseclickEvent, offcanvasTriggerHandler));
  };
  const toggleOffCanvasDismiss = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const doc = shorty.getDocument(self.element);
    action(doc, shorty.keydownEvent, offcanvasKeyDismissHandler);
    action(doc, shorty.mouseclickEvent, offcanvasDismissHandler);
  };
  const beforeOffcanvasShow = (self) => {
    const { element, options } = self;
    if (!options.scroll) {
      setOffCanvasScrollbar(self);
      shorty.setElementStyle(shorty.getDocumentBody(element), { overflow: "hidden" });
    }
    shorty.addClass(element, offcanvasTogglingClass);
    shorty.addClass(element, showClass);
    shorty.setElementStyle(element, { visibility: "visible" });
    shorty.emulateTransitionEnd(element, () => showOffcanvasComplete(self));
  };
  const beforeOffcanvasHide = (self, callback) => {
    const { element, options } = self;
    const currentOpen = getCurrentOpen(element);
    element.blur();
    if (!currentOpen && options.backdrop && shorty.hasClass(overlay, showClass)) {
      hideOverlay();
      shorty.emulateTransitionEnd(overlay, () => hideOffcanvasComplete(self, callback));
    } else
      hideOffcanvasComplete(self, callback);
  };
  const offcanvasTriggerHandler = (e) => {
    const trigger = shorty.closest(e.target, offcanvasToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getOffcanvasInstance(element);
    if (self) {
      self.relatedTarget = trigger;
      self.toggle();
      if (trigger && trigger.tagName === "A") {
        e.preventDefault();
      }
    }
  };
  const offcanvasDismissHandler = (e) => {
    const { target } = e;
    const element = shorty.querySelector(offcanvasActiveSelector, shorty.getDocument(target));
    const offCanvasDismiss = shorty.querySelector(offcanvasDismissSelector, element);
    const self = element && getOffcanvasInstance(element);
    if (!self)
      return;
    const { options, triggers } = self;
    const { backdrop } = options;
    const trigger = shorty.closest(target, offcanvasToggleSelector);
    const selection = shorty.getDocument(element).getSelection();
    if (overlay.contains(target) && backdrop === "static")
      return;
    if (!(selection && selection.toString().length) && (!element.contains(target) && backdrop && /* istanbul ignore next */
    (!trigger || triggers.includes(target)) || offCanvasDismiss && offCanvasDismiss.contains(target))) {
      self.relatedTarget = offCanvasDismiss && offCanvasDismiss.contains(target) ? offCanvasDismiss : null;
      self.hide();
    }
    if (trigger && trigger.tagName === "A")
      e.preventDefault();
  };
  const offcanvasKeyDismissHandler = ({ code, target }) => {
    const element = shorty.querySelector(offcanvasActiveSelector, shorty.getDocument(target));
    const self = element && getOffcanvasInstance(element);
    if (!self)
      return;
    if (self.options.keyboard && code === shorty.keyEscape) {
      self.relatedTarget = null;
      self.hide();
    }
  };
  const showOffcanvasComplete = (self) => {
    const { element } = self;
    shorty.removeClass(element, offcanvasTogglingClass);
    shorty.removeAttribute(element, shorty.ariaHidden);
    shorty.setAttribute(element, shorty.ariaModal, "true");
    shorty.setAttribute(element, "role", "dialog");
    shorty.dispatchEvent(element, shownOffcanvasEvent);
    toggleOffCanvasDismiss(self, true);
    shorty.focus(element);
  };
  const hideOffcanvasComplete = (self, callback) => {
    const { element, triggers } = self;
    shorty.setAttribute(element, shorty.ariaHidden, "true");
    shorty.removeAttribute(element, shorty.ariaModal);
    shorty.removeAttribute(element, "role");
    shorty.setElementStyle(element, { visibility: "" });
    const visibleTrigger = showOffcanvasEvent.relatedTarget || triggers.find(isVisible);
    if (visibleTrigger)
      shorty.focus(visibleTrigger);
    removeOverlay(element);
    shorty.dispatchEvent(element, hiddenOffcanvasEvent);
    shorty.removeClass(element, offcanvasTogglingClass);
    if (!getCurrentOpen(element)) {
      toggleOffCanvasDismiss(self);
    }
    if (shorty.isFunction(callback))
      callback();
  };
  class Offcanvas extends BaseComponent {
    /**
     * @param target usually an `.offcanvas` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.triggers = [...shorty.querySelectorAll(offcanvasToggleSelector, shorty.getDocument(element))].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.relatedTarget = null;
      toggleOffcanvasEvents(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return offcanvasComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return offcanvasDefaults;
    }
    // OFFCANVAS PUBLIC METHODS
    // ========================
    /** Shows or hides the offcanvas from the user. */
    toggle() {
      if (shorty.hasClass(this.element, showClass))
        this.hide();
      else
        this.show();
    }
    /** Shows the offcanvas to the user. */
    show() {
      const { element, options, relatedTarget } = this;
      let overlayDelay = 0;
      if (shorty.hasClass(element, showClass))
        return;
      showOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      shownOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      shorty.dispatchEvent(element, showOffcanvasEvent);
      if (showOffcanvasEvent.defaultPrevented)
        return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getOffcanvasInstance(currentOpen) || /* istanbul ignore next */
        shorty.getInstance(currentOpen, modalComponent);
        if (that)
          that.hide();
      }
      if (options.backdrop) {
        if (!hasPopup(overlay)) {
          appendOverlay(element, true);
        } else {
          toggleOverlayType();
        }
        overlayDelay = shorty.getElementTransitionDuration(overlay);
        showOverlay();
        setTimeout(() => beforeOffcanvasShow(this), overlayDelay);
      } else {
        beforeOffcanvasShow(this);
        if (currentOpen && shorty.hasClass(overlay, showClass)) {
          hideOverlay();
        }
      }
    }
    /**
     * Hides the offcanvas from the user.
     *
     * @param callback when `true` it will skip animation
     */
    hide(callback) {
      const { element, relatedTarget } = this;
      if (!shorty.hasClass(element, showClass))
        return;
      hideOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      hiddenOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      shorty.dispatchEvent(element, hideOffcanvasEvent);
      if (hideOffcanvasEvent.defaultPrevented)
        return;
      shorty.addClass(element, offcanvasTogglingClass);
      shorty.removeClass(element, showClass);
      if (!callback) {
        shorty.emulateTransitionEnd(element, () => beforeOffcanvasHide(this, callback));
      } else
        beforeOffcanvasHide(this, callback);
    }
    /** Removes the `Offcanvas` from the target element. */
    dispose() {
      toggleOffcanvasEvents(this);
      this.hide(() => super.dispose());
    }
  }
  __publicField(Offcanvas, "selector", offcanvasSelector);
  __publicField(Offcanvas, "init", offcanvasInitCallback);
  __publicField(Offcanvas, "getInstance", getOffcanvasInstance);
  const popoverString = "popover";
  const popoverComponent = "Popover";
  const tooltipString = "tooltip";
  const getTipTemplate = (tipType) => {
    const isTooltip = tipType === tooltipString;
    const bodyClass = isTooltip ? `${tipType}-inner` : `${tipType}-body`;
    const header = !isTooltip ? `<h3 class="${tipType}-header"></h3>` : "";
    const arrow = `<div class="${tipType}-arrow"></div>`;
    const body = `<div class="${bodyClass}"></div>`;
    return `<div class="${tipType}" role="${tooltipString}">${header + arrow + body}</div>`;
  };
  const tipClassPositions = {
    top: "top",
    bottom: "bottom",
    left: "start",
    right: "end"
  };
  const styleTip = (self) => {
    const tipClasses = /\b(top|bottom|start|end)+/;
    const { element, tooltip, container, options, arrow } = self;
    if (!tooltip)
      return;
    const tipPositions = { ...tipClassPositions };
    const RTL = shorty.isRTL(element);
    shorty.setElementStyle(tooltip, {
      // top: '0px', left: '0px', right: '', bottom: '',
      top: "",
      left: "",
      right: "",
      bottom: ""
    });
    const isPopover = self.name === popoverComponent;
    const { offsetWidth: tipWidth, offsetHeight: tipHeight } = tooltip;
    const { clientWidth: htmlcw, clientHeight: htmlch, offsetWidth: htmlow } = shorty.getDocumentElement(element);
    let { placement } = options;
    const { clientWidth: parentCWidth, offsetWidth: parentOWidth } = container;
    const parentPosition = shorty.getElementStyle(container, "position");
    const fixedParent = parentPosition === "fixed";
    const scrollbarWidth = fixedParent ? Math.abs(parentCWidth - parentOWidth) : Math.abs(htmlcw - htmlow);
    const leftBoundry = RTL && fixedParent ? (
      /* istanbul ignore next */
      scrollbarWidth
    ) : 0;
    const rightBoundry = htmlcw - (!RTL ? scrollbarWidth : 0) - 1;
    const {
      width: elemWidth,
      height: elemHeight,
      left: elemRectLeft,
      right: elemRectRight,
      top: elemRectTop
    } = shorty.getBoundingClientRect(element, true);
    const { x, y } = {
      x: elemRectLeft,
      y: elemRectTop
    };
    shorty.setElementStyle(arrow, {
      top: "",
      left: "",
      right: "",
      bottom: ""
    });
    let topPosition = 0;
    let bottomPosition = "";
    let leftPosition = 0;
    let rightPosition = "";
    let arrowTop = "";
    let arrowLeft = "";
    let arrowRight = "";
    const arrowWidth = arrow.offsetWidth || 0;
    const arrowHeight = arrow.offsetHeight || 0;
    const arrowAdjust = arrowWidth / 2;
    let topExceed = elemRectTop - tipHeight - arrowHeight < 0;
    let bottomExceed = elemRectTop + tipHeight + elemHeight + arrowHeight >= htmlch;
    let leftExceed = elemRectLeft - tipWidth - arrowWidth < leftBoundry;
    let rightExceed = elemRectLeft + tipWidth + elemWidth + arrowWidth >= rightBoundry;
    const horizontals = ["left", "right"];
    const verticals = ["top", "bottom"];
    topExceed = horizontals.includes(placement) ? elemRectTop + elemHeight / 2 - tipHeight / 2 - arrowHeight < 0 : topExceed;
    bottomExceed = horizontals.includes(placement) ? elemRectTop + tipHeight / 2 + elemHeight / 2 + arrowHeight >= htmlch : bottomExceed;
    leftExceed = verticals.includes(placement) ? elemRectLeft + elemWidth / 2 - tipWidth / 2 < leftBoundry : leftExceed;
    rightExceed = verticals.includes(placement) ? elemRectLeft + tipWidth / 2 + elemWidth / 2 >= rightBoundry : rightExceed;
    placement = horizontals.includes(placement) && leftExceed && rightExceed ? "top" : placement;
    placement = placement === "top" && topExceed ? "bottom" : placement;
    placement = placement === "bottom" && bottomExceed ? "top" : placement;
    placement = placement === "left" && leftExceed ? "right" : placement;
    placement = placement === "right" && rightExceed ? "left" : placement;
    if (!tooltip.className.includes(placement)) {
      tooltip.className = tooltip.className.replace(tipClasses, tipPositions[placement]);
    }
    if (horizontals.includes(placement)) {
      if (placement === "left") {
        leftPosition = x - tipWidth - (isPopover ? arrowWidth : 0);
      } else {
        leftPosition = x + elemWidth + (isPopover ? arrowWidth : 0);
      }
      if (topExceed && bottomExceed) {
        topPosition = 0;
        bottomPosition = 0;
        arrowTop = elemRectTop + elemHeight / 2 - arrowHeight / 2;
      } else if (topExceed) {
        topPosition = y;
        bottomPosition = "";
        arrowTop = elemHeight / 2 - arrowWidth;
      } else if (bottomExceed) {
        topPosition = y - tipHeight + elemHeight;
        bottomPosition = "";
        arrowTop = tipHeight - elemHeight / 2 - arrowWidth;
      } else {
        topPosition = y - tipHeight / 2 + elemHeight / 2;
        arrowTop = tipHeight / 2 - arrowHeight / 2;
      }
    } else if (verticals.includes(placement)) {
      if (placement === "top") {
        topPosition = y - tipHeight - (isPopover ? arrowHeight : 0);
      } else {
        topPosition = y + elemHeight + (isPopover ? arrowHeight : 0);
      }
      if (leftExceed) {
        leftPosition = 0;
        arrowLeft = x + elemWidth / 2 - arrowAdjust;
      } else if (rightExceed) {
        leftPosition = "auto";
        rightPosition = 0;
        arrowRight = elemWidth / 2 + rightBoundry - elemRectRight - arrowAdjust;
      } else {
        leftPosition = x - tipWidth / 2 + elemWidth / 2;
        arrowLeft = tipWidth / 2 - arrowAdjust;
      }
    }
    shorty.setElementStyle(tooltip, {
      top: `${topPosition}px`,
      bottom: bottomPosition === "" ? "" : `${bottomPosition}px`,
      left: leftPosition === "auto" ? leftPosition : `${leftPosition}px`,
      right: rightPosition !== "" ? `${rightPosition}px` : ""
    });
    if (shorty.isHTMLElement(arrow)) {
      if (arrowTop !== "") {
        arrow.style.top = `${arrowTop}px`;
      }
      if (arrowLeft !== "") {
        arrow.style.left = `${arrowLeft}px`;
      } else if (arrowRight !== "") {
        arrow.style.right = `${arrowRight}px`;
      }
    }
    const updatedTooltipEvent = shorty.createCustomEvent(`updated.bs.${shorty.toLowerCase(self.name)}`);
    shorty.dispatchEvent(element, updatedTooltipEvent);
  };
  const tooltipDefaults = {
    template: getTipTemplate(tooltipString),
    title: "",
    customClass: "",
    trigger: "hover focus",
    placement: "top",
    sanitizeFn: void 0,
    animation: true,
    delay: 200,
    container: document.body,
    content: "",
    dismissible: false,
    btnClose: ""
  };
  const dataOriginalTitle = "data-original-title";
  const tooltipComponent = "Tooltip";
  const setHtml = (element, content, sanitizeFn) => {
    if (!shorty.isHTMLElement(element) || shorty.isString(content) && !content.length)
      return;
    if (shorty.isString(content)) {
      let dirty = content.trim();
      if (shorty.isFunction(sanitizeFn))
        dirty = sanitizeFn(dirty);
      const domParser = new DOMParser();
      const tempDocument = domParser.parseFromString(dirty, "text/html");
      element.append(...[...tempDocument.body.childNodes]);
    } else if (shorty.isHTMLElement(content)) {
      element.append(content);
    } else if (shorty.isNodeList(content) || shorty.isArray(content) && content.every(shorty.isNode)) {
      element.append(...[...content]);
    }
  };
  const createTip = (self) => {
    const isTooltip = self.name === tooltipComponent;
    const { id, element, options } = self;
    const { title, placement, template, animation, customClass, sanitizeFn, dismissible, content, btnClose } = options;
    const tipString = isTooltip ? tooltipString : popoverString;
    const tipPositions = { ...tipClassPositions };
    let titleParts = [];
    let contentParts = [];
    if (shorty.isRTL(element)) {
      tipPositions.left = "end";
      tipPositions.right = "start";
    }
    const placementClass = `bs-${tipString}-${tipPositions[placement]}`;
    let tooltipTemplate;
    if (shorty.isHTMLElement(template)) {
      tooltipTemplate = template;
    } else {
      const htmlMarkup = shorty.createElement("div");
      setHtml(htmlMarkup, template, sanitizeFn);
      tooltipTemplate = htmlMarkup.firstChild;
    }
    self.tooltip = shorty.isHTMLElement(tooltipTemplate) ? tooltipTemplate.cloneNode(true) : (
      /* istanbul ignore next */
      void 0
    );
    const { tooltip } = self;
    if (!tooltip)
      return;
    shorty.setAttribute(tooltip, "id", id);
    shorty.setAttribute(tooltip, "role", tooltipString);
    const bodyClass = isTooltip ? `${tooltipString}-inner` : `${popoverString}-body`;
    const tooltipHeader = isTooltip ? null : shorty.querySelector(`.${popoverString}-header`, tooltip);
    const tooltipBody = shorty.querySelector(`.${bodyClass}`, tooltip);
    self.arrow = shorty.querySelector(`.${tipString}-arrow`, tooltip);
    const { arrow } = self;
    if (shorty.isHTMLElement(title))
      titleParts = [title.cloneNode(true)];
    else {
      const tempTitle = shorty.createElement("div");
      setHtml(tempTitle, title, sanitizeFn);
      titleParts = [...[...tempTitle.childNodes]];
    }
    if (shorty.isHTMLElement(content))
      contentParts = [content.cloneNode(true)];
    else {
      const tempContent = shorty.createElement("div");
      setHtml(tempContent, content, sanitizeFn);
      contentParts = [...[...tempContent.childNodes]];
    }
    if (dismissible) {
      if (title) {
        if (shorty.isHTMLElement(btnClose))
          titleParts = [...titleParts, btnClose.cloneNode(true)];
        else {
          const tempBtn = shorty.createElement("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          titleParts = [...titleParts, tempBtn.firstChild];
        }
      } else {
        if (tooltipHeader)
          tooltipHeader.remove();
        if (shorty.isHTMLElement(btnClose))
          contentParts = [...contentParts, btnClose.cloneNode(true)];
        else {
          const tempBtn = shorty.createElement("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          contentParts = [...contentParts, tempBtn.firstChild];
        }
      }
    }
    if (!isTooltip) {
      if (title && tooltipHeader)
        setHtml(tooltipHeader, titleParts, sanitizeFn);
      if (content && tooltipBody)
        setHtml(tooltipBody, contentParts, sanitizeFn);
      self.btn = shorty.querySelector(".btn-close", tooltip) || void 0;
    } else if (title && tooltipBody)
      setHtml(tooltipBody, title, sanitizeFn);
    shorty.addClass(tooltip, "position-fixed");
    shorty.addClass(arrow, "position-absolute");
    if (!shorty.hasClass(tooltip, tipString))
      shorty.addClass(tooltip, tipString);
    if (animation && !shorty.hasClass(tooltip, fadeClass))
      shorty.addClass(tooltip, fadeClass);
    if (customClass && !shorty.hasClass(tooltip, customClass)) {
      shorty.addClass(tooltip, customClass);
    }
    if (!shorty.hasClass(tooltip, placementClass))
      shorty.addClass(tooltip, placementClass);
  };
  const getElementContainer = (element) => {
    const majorBlockTags = ["HTML", "BODY"];
    const containers = [];
    let { parentNode } = element;
    while (parentNode && !majorBlockTags.includes(parentNode.nodeName)) {
      parentNode = shorty.getParentNode(parentNode);
      if (!(shorty.isShadowRoot(parentNode) || shorty.isTableElement(parentNode))) {
        containers.push(parentNode);
      }
    }
    return containers.find((c, i) => {
      if (shorty.getElementStyle(c, "position") !== "relative" && containers.slice(i + 1).every((r) => shorty.getElementStyle(r, "position") === "static")) {
        return c;
      }
      return null;
    }) || /* istanbul ignore next: optional guard */
    shorty.getDocument(element).body;
  };
  const tooltipSelector = `[${dataBsToggle}="${tooltipString}"],[data-tip="${tooltipString}"]`;
  const titleAttr = "title";
  let getTooltipInstance = (element) => shorty.getInstance(element, tooltipComponent);
  const tooltipInitCallback = (element) => new Tooltip(element);
  const removeTooltip = (self) => {
    const { element, tooltip, container, offsetParent } = self;
    shorty.removeAttribute(element, shorty.ariaDescribedBy);
    removePopup(tooltip, container === offsetParent ? container : offsetParent);
  };
  const disposeTooltipComplete = (self, callback) => {
    const { element } = self;
    toggleTooltipHandlers(self);
    if (shorty.hasAttribute(element, dataOriginalTitle) && self.name === tooltipComponent) {
      toggleTooltipTitle(self);
    }
    if (callback)
      callback();
  };
  const toggleTooltipAction = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { element } = self;
    action(shorty.getDocument(element), shorty.touchstartEvent, self.handleTouch, shorty.passiveHandler);
    [shorty.scrollEvent, shorty.resizeEvent].forEach((ev) => {
      action(shorty.getWindow(element), ev, self.update, shorty.passiveHandler);
    });
  };
  const tooltipShownAction = (self) => {
    const { element } = self;
    const shownTooltipEvent = shorty.createCustomEvent(`shown.bs.${shorty.toLowerCase(self.name)}`);
    toggleTooltipAction(self, true);
    shorty.dispatchEvent(element, shownTooltipEvent);
    shorty.Timer.clear(element, "in");
  };
  const tooltipHiddenAction = (self) => {
    const { element, onHideComplete } = self;
    const hiddenTooltipEvent = shorty.createCustomEvent(`hidden.bs.${shorty.toLowerCase(self.name)}`);
    toggleTooltipAction(self);
    removeTooltip(self);
    shorty.dispatchEvent(element, hiddenTooltipEvent);
    if (shorty.isFunction(onHideComplete)) {
      onHideComplete();
      self.onHideComplete = void 0;
    }
    shorty.Timer.clear(element, "out");
  };
  const toggleTooltipHandlers = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { element, options, btn } = self;
    const { trigger } = options;
    const isPopover = self.name !== tooltipComponent;
    const dismissible = isPopover && options.dismissible ? true : false;
    if (trigger.includes("manual"))
      return;
    self.enabled = !!add;
    const triggerOptions = trigger.split(" ");
    triggerOptions.forEach((tr) => {
      if (tr === shorty.mousehoverEvent) {
        action(element, shorty.mousedownEvent, self.show);
        action(element, shorty.mouseenterEvent, self.show);
        if (dismissible && btn) {
          action(btn, shorty.mouseclickEvent, self.hide);
        } else {
          action(element, shorty.mouseleaveEvent, self.hide);
          action(shorty.getDocument(element), shorty.touchstartEvent, self.handleTouch, shorty.passiveHandler);
        }
      } else if (tr === shorty.mouseclickEvent) {
        action(element, tr, !dismissible ? self.toggle : self.show);
      } else if (tr === shorty.focusEvent) {
        action(element, shorty.focusinEvent, self.show);
        if (!dismissible)
          action(element, shorty.focusoutEvent, self.hide);
        if (shorty.isApple) {
          action(element, shorty.mouseclickEvent, () => shorty.focus(element));
        }
      }
    });
  };
  const toggleTooltipOpenHandlers = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { element, container, offsetParent } = self;
    const { offsetHeight, scrollHeight } = container;
    const parentModal = shorty.closest(element, `.${modalString}`);
    const parentOffcanvas = shorty.closest(element, `.${offcanvasString}`);
    const win = shorty.getWindow(element);
    const overflow = offsetHeight !== scrollHeight;
    const scrollTarget = container === offsetParent && overflow ? container : win;
    action(scrollTarget, shorty.resizeEvent, self.update, shorty.passiveHandler);
    action(scrollTarget, shorty.scrollEvent, self.update, shorty.passiveHandler);
    if (parentModal)
      action(parentModal, `hide.bs.${modalString}`, self.hide);
    if (parentOffcanvas)
      action(parentOffcanvas, `hide.bs.${offcanvasString}`, self.hide);
  };
  const toggleTooltipTitle = (self, content) => {
    const titleAtt = [dataOriginalTitle, titleAttr];
    const { element } = self;
    shorty.setAttribute(
      element,
      titleAtt[content ? 0 : 1],
      content || shorty.getAttribute(element, titleAtt[0]) || /* istanbul ignore next */
      ""
    );
    shorty.removeAttribute(element, titleAtt[content ? 1 : 0]);
  };
  class Tooltip extends BaseComponent {
    /**
     * @param target the target element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      // TOOLTIP PUBLIC METHODS
      // ======================
      /** Shows the tooltip. */
      __publicField(this, "show", () => this._show());
      /** Hides the tooltip. */
      __publicField(this, "hide", () => {
        const { options, tooltip, element, container, offsetParent } = this;
        const { animation, delay } = options;
        shorty.Timer.clear(element, "in");
        if (tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent)) {
          shorty.Timer.set(
            element,
            () => {
              const hideTooltipEvent = shorty.createCustomEvent(`hide.bs.${shorty.toLowerCase(this.name)}`);
              shorty.dispatchEvent(element, hideTooltipEvent);
              if (hideTooltipEvent.defaultPrevented)
                return;
              this.update();
              shorty.removeClass(tooltip, showClass);
              toggleTooltipOpenHandlers(this);
              if (animation)
                shorty.emulateTransitionEnd(tooltip, () => tooltipHiddenAction(this));
              else
                tooltipHiddenAction(this);
            },
            delay + 17,
            "out"
          );
        }
      });
      /** Updates the tooltip position. */
      __publicField(this, "update", () => {
        styleTip(this);
      });
      /** Toggles the tooltip visibility. */
      __publicField(this, "toggle", () => {
        const { tooltip, container, offsetParent } = this;
        if (tooltip && !hasPopup(tooltip, container === offsetParent ? container : offsetParent))
          this.show();
        else
          this.hide();
      });
      /**
       * Handles the `touchstart` event listener for `Tooltip`
       *
       * @this {Tooltip}
       * @param {TouchEvent} e the `Event` object
       */
      __publicField(this, "handleTouch", ({ target }) => {
        const { tooltip, element } = this;
        if (tooltip && tooltip.contains(target) || target === element || target && element.contains(target))
          ;
        else {
          this.hide();
        }
      });
      const { element } = this;
      const isTooltip = this.name === tooltipComponent;
      const tipString = isTooltip ? tooltipString : popoverString;
      const tipComponent = isTooltip ? tooltipComponent : popoverComponent;
      getTooltipInstance = (elem) => shorty.getInstance(elem, tipComponent);
      this.enabled = true;
      this.id = `${tipString}-${shorty.getUID(element, tipString)}`;
      const { options } = this;
      if (!options.title && isTooltip || !isTooltip && !options.content) {
        return;
      }
      shorty.ObjectAssign(tooltipDefaults, { titleAttr: "" });
      if (shorty.hasAttribute(element, titleAttr) && isTooltip && typeof options.title === "string") {
        toggleTooltipTitle(this, options.title);
      }
      this.container = getElementContainer(element);
      this.offsetParent = ["sticky", "fixed"].some(
        (position) => shorty.getElementStyle(this.container, "position") === position
      ) ? this.container : shorty.getDocument(this.element).body;
      createTip(this);
      toggleTooltipHandlers(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return tooltipComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return tooltipDefaults;
    }
    _show() {
      const { options, tooltip, element, container, offsetParent, id } = this;
      const { animation } = options;
      const outTimer = shorty.Timer.get(element, "out");
      const tipContainer = container === offsetParent ? container : offsetParent;
      shorty.Timer.clear(element, "out");
      if (tooltip && !outTimer && !hasPopup(tooltip, tipContainer)) {
        shorty.Timer.set(
          element,
          () => {
            const showTooltipEvent = shorty.createCustomEvent(`show.bs.${shorty.toLowerCase(this.name)}`);
            shorty.dispatchEvent(element, showTooltipEvent);
            if (showTooltipEvent.defaultPrevented)
              return;
            appendPopup(tooltip, tipContainer);
            shorty.setAttribute(element, shorty.ariaDescribedBy, `#${id}`);
            this.update();
            toggleTooltipOpenHandlers(this, true);
            if (!shorty.hasClass(tooltip, showClass))
              shorty.addClass(tooltip, showClass);
            if (animation)
              shorty.emulateTransitionEnd(tooltip, () => tooltipShownAction(this));
            else
              tooltipShownAction(this);
          },
          17,
          "in"
        );
      }
    }
    /** Enables the tooltip. */
    enable() {
      const { enabled } = this;
      if (!enabled) {
        toggleTooltipHandlers(this, true);
        this.enabled = !enabled;
      }
    }
    /** Disables the tooltip. */
    disable() {
      const { tooltip, container, offsetParent, options, enabled } = this;
      const { animation } = options;
      if (enabled) {
        if (tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent) && animation) {
          this.onHideComplete = () => toggleTooltipHandlers(this);
          this.hide();
        } else {
          toggleTooltipHandlers(this);
        }
        this.enabled = !enabled;
      }
    }
    /** Toggles the `disabled` property. */
    toggleEnabled() {
      if (!this.enabled)
        this.enable();
      else
        this.disable();
    }
    /** Removes the `Tooltip` from the target element. */
    dispose() {
      const { tooltip, container, offsetParent, options } = this;
      const callback = () => disposeTooltipComplete(this, () => super.dispose());
      if (options.animation && tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent)) {
        this.options.delay = 0;
        this.onHideComplete = callback;
        this.hide();
      } else {
        callback();
      }
    }
  }
  __publicField(Tooltip, "selector", tooltipSelector);
  __publicField(Tooltip, "init", tooltipInitCallback);
  __publicField(Tooltip, "getInstance", getTooltipInstance);
  __publicField(Tooltip, "styleTip", styleTip);
  const popoverSelector = `[${dataBsToggle}="${popoverString}"],[data-tip="${popoverString}"]`;
  const popoverDefaults = shorty.ObjectAssign({}, tooltipDefaults, {
    template: getTipTemplate(popoverString),
    content: "",
    dismissible: false,
    btnClose: '<button class="btn-close" aria-label="Close"></button>'
  });
  const getPopoverInstance = (element) => shorty.getInstance(element, popoverComponent);
  const popoverInitCallback = (element) => new Popover(element);
  class Popover extends Tooltip {
    /**
     * @param target the target element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      /* extend original `show()` */
      __publicField(this, "show", () => {
        super._show();
        const { options, btn } = this;
        if (options.dismissible && btn)
          setTimeout(() => shorty.focus(btn), 17);
      });
    }
    /**
     * Returns component name string.
     */
    get name() {
      return popoverComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return popoverDefaults;
    }
  }
  __publicField(Popover, "selector", popoverSelector);
  __publicField(Popover, "init", popoverInitCallback);
  __publicField(Popover, "getInstance", getPopoverInstance);
  __publicField(Popover, "styleTip", styleTip);
  const tabString = "tab";
  const tabComponent = "Tab";
  const tabSelector = `[${dataBsToggle}="${tabString}"]`;
  const getTabInstance = (element) => shorty.getInstance(element, tabComponent);
  const tabInitCallback = (element) => new Tab(element);
  const showTabEvent = shorty.createCustomEvent(`show.bs.${tabString}`);
  const shownTabEvent = shorty.createCustomEvent(`shown.bs.${tabString}`);
  const hideTabEvent = shorty.createCustomEvent(`hide.bs.${tabString}`);
  const hiddenTabEvent = shorty.createCustomEvent(`hidden.bs.${tabString}`);
  const tabPrivate = /* @__PURE__ */ new Map();
  const triggerTabEnd = (self) => {
    const { tabContent, nav } = self;
    if (tabContent && shorty.hasClass(tabContent, collapsingClass)) {
      tabContent.style.height = "";
      shorty.removeClass(tabContent, collapsingClass);
    }
    if (nav)
      shorty.Timer.clear(nav);
  };
  const triggerTabShow = (self) => {
    const { element, tabContent, content: nextContent, nav } = self;
    const { tab } = shorty.isHTMLElement(nav) && tabPrivate.get(nav) || /* istanbul ignore next */
    { tab: null };
    if (tabContent && nextContent && shorty.hasClass(nextContent, fadeClass)) {
      const { currentHeight, nextHeight } = tabPrivate.get(element) || /* istanbul ignore next */
      {
        currentHeight: 0,
        nextHeight: 0
      };
      if (currentHeight === nextHeight) {
        triggerTabEnd(self);
      } else {
        setTimeout(() => {
          tabContent.style.height = `${nextHeight}px`;
          shorty.reflow(tabContent);
          shorty.emulateTransitionEnd(tabContent, () => triggerTabEnd(self));
        }, 50);
      }
    } else if (nav)
      shorty.Timer.clear(nav);
    shownTabEvent.relatedTarget = tab;
    shorty.dispatchEvent(element, shownTabEvent);
  };
  const triggerTabHide = (self) => {
    const { element, content: nextContent, tabContent, nav } = self;
    const { tab, content } = nav && tabPrivate.get(nav) || /* istanbul ignore next */
    { tab: null, content: null };
    let currentHeight = 0;
    if (tabContent && nextContent && shorty.hasClass(nextContent, fadeClass)) {
      [content, nextContent].forEach((c) => {
        if (shorty.isHTMLElement(c))
          shorty.addClass(c, "overflow-hidden");
      });
      currentHeight = shorty.isHTMLElement(content) ? content.scrollHeight : (
        /* istanbul ignore next */
        0
      );
    }
    showTabEvent.relatedTarget = tab;
    hiddenTabEvent.relatedTarget = element;
    shorty.dispatchEvent(element, showTabEvent);
    if (showTabEvent.defaultPrevented)
      return;
    if (nextContent)
      shorty.addClass(nextContent, activeClass);
    if (content)
      shorty.removeClass(content, activeClass);
    if (tabContent && nextContent && shorty.hasClass(nextContent, fadeClass)) {
      const nextHeight = nextContent.scrollHeight;
      tabPrivate.set(element, { currentHeight, nextHeight, tab: null, content: null });
      shorty.addClass(tabContent, collapsingClass);
      tabContent.style.height = `${currentHeight}px`;
      shorty.reflow(tabContent);
      [content, nextContent].forEach((c) => {
        if (c)
          shorty.removeClass(c, "overflow-hidden");
      });
    }
    if (nextContent && nextContent && shorty.hasClass(nextContent, fadeClass)) {
      setTimeout(() => {
        shorty.addClass(nextContent, showClass);
        shorty.emulateTransitionEnd(nextContent, () => {
          triggerTabShow(self);
        });
      }, 1);
    } else {
      if (nextContent)
        shorty.addClass(nextContent, showClass);
      triggerTabShow(self);
    }
    if (tab)
      shorty.dispatchEvent(tab, hiddenTabEvent);
  };
  const getActiveTab = (self) => {
    const { nav } = self;
    if (!shorty.isHTMLElement(nav))
      return { tab: null, content: null };
    const activeTabs = shorty.getElementsByClassName(activeClass, nav);
    let tab = null;
    if (activeTabs.length === 1 && !dropdownMenuClasses.some((c) => shorty.hasClass(activeTabs[0].parentElement, c))) {
      [tab] = activeTabs;
    } else if (activeTabs.length > 1) {
      tab = activeTabs[activeTabs.length - 1];
    }
    const content = shorty.isHTMLElement(tab) ? getTargetElement(tab) : null;
    return { tab, content };
  };
  const getParentDropdown = (element) => {
    if (!shorty.isHTMLElement(element))
      return null;
    const dropdown = shorty.closest(element, `.${dropdownMenuClasses.join(",.")}`);
    return dropdown ? shorty.querySelector(`.${dropdownMenuClasses[0]}-toggle`, dropdown) : null;
  };
  const toggleTabHandler = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    action(self.element, shorty.mouseclickEvent, tabClickHandler);
  };
  const tabClickHandler = (e) => {
    const self = getTabInstance(e.target);
    if (!self)
      return;
    e.preventDefault();
    self.show();
  };
  class Tab extends BaseComponent {
    /** @param target the target element */
    constructor(target) {
      super(target);
      const { element } = this;
      const content = getTargetElement(element);
      if (!content)
        return;
      const nav = shorty.closest(element, ".nav");
      const container = shorty.closest(content, ".tab-content");
      this.nav = nav;
      this.content = content;
      this.tabContent = container;
      this.dropdown = getParentDropdown(element);
      const { tab } = getActiveTab(this);
      if (nav && !tab) {
        const firstTab = shorty.querySelector(tabSelector, nav);
        const firstTabContent = firstTab && getTargetElement(firstTab);
        if (firstTabContent) {
          shorty.addClass(firstTab, activeClass);
          shorty.addClass(firstTabContent, showClass);
          shorty.addClass(firstTabContent, activeClass);
          shorty.setAttribute(element, shorty.ariaSelected, "true");
        }
      }
      toggleTabHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return tabComponent;
    }
    // TAB PUBLIC METHODS
    // ==================
    /** Shows the tab to the user. */
    show() {
      const { element, content: nextContent, nav, dropdown } = this;
      if (!(nav && shorty.Timer.get(nav)) && !shorty.hasClass(element, activeClass)) {
        const { tab, content } = getActiveTab(this);
        if (nav)
          tabPrivate.set(nav, { tab, content, currentHeight: 0, nextHeight: 0 });
        hideTabEvent.relatedTarget = element;
        if (shorty.isHTMLElement(tab))
          shorty.dispatchEvent(tab, hideTabEvent);
        if (hideTabEvent.defaultPrevented)
          return;
        shorty.addClass(element, activeClass);
        shorty.setAttribute(element, shorty.ariaSelected, "true");
        const activeDropdown = shorty.isHTMLElement(tab) && getParentDropdown(tab);
        if (activeDropdown && shorty.hasClass(activeDropdown, activeClass)) {
          shorty.removeClass(activeDropdown, activeClass);
        }
        if (nav) {
          const toggleTab = () => {
            if (tab) {
              shorty.removeClass(tab, activeClass);
              shorty.setAttribute(tab, shorty.ariaSelected, "false");
            }
            if (dropdown && !shorty.hasClass(dropdown, activeClass))
              shorty.addClass(dropdown, activeClass);
          };
          if (content && (shorty.hasClass(content, fadeClass) || nextContent && shorty.hasClass(nextContent, fadeClass))) {
            shorty.Timer.set(nav, toggleTab, 1);
          } else
            toggleTab();
        }
        if (content) {
          shorty.removeClass(content, showClass);
          if (shorty.hasClass(content, fadeClass)) {
            shorty.emulateTransitionEnd(content, () => triggerTabHide(this));
          } else {
            triggerTabHide(this);
          }
        }
      }
    }
    /** Removes the `Tab` component from the target element. */
    dispose() {
      toggleTabHandler(this);
      super.dispose();
    }
  }
  __publicField(Tab, "selector", tabSelector);
  __publicField(Tab, "init", tabInitCallback);
  __publicField(Tab, "getInstance", getTabInstance);
  const toastString = "toast";
  const toastComponent = "Toast";
  const toastSelector = `.${toastString}`;
  const toastDismissSelector = `[${dataBsDismiss}="${toastString}"]`;
  const toastToggleSelector = `[${dataBsToggle}="${toastString}"]`;
  const showingClass = "showing";
  const hideClass = "hide";
  const toastDefaults = {
    animation: true,
    autohide: true,
    delay: 5e3
  };
  const getToastInstance = (element) => shorty.getInstance(element, toastComponent);
  const toastInitCallback = (element) => new Toast(element);
  const showToastEvent = shorty.createCustomEvent(`show.bs.${toastString}`);
  const shownToastEvent = shorty.createCustomEvent(`shown.bs.${toastString}`);
  const hideToastEvent = shorty.createCustomEvent(`hide.bs.${toastString}`);
  const hiddenToastEvent = shorty.createCustomEvent(`hidden.bs.${toastString}`);
  const showToastComplete = (self) => {
    const { element, options } = self;
    shorty.removeClass(element, showingClass);
    shorty.Timer.clear(element, showingClass);
    shorty.dispatchEvent(element, shownToastEvent);
    if (options.autohide) {
      shorty.Timer.set(element, () => self.hide(), options.delay, toastString);
    }
  };
  const hideToastComplete = (self) => {
    const { element } = self;
    shorty.removeClass(element, showingClass);
    shorty.removeClass(element, showClass);
    shorty.addClass(element, hideClass);
    shorty.Timer.clear(element, toastString);
    shorty.dispatchEvent(element, hiddenToastEvent);
  };
  const hideToast = (self) => {
    const { element, options } = self;
    shorty.addClass(element, showingClass);
    if (options.animation) {
      shorty.reflow(element);
      shorty.emulateTransitionEnd(element, () => hideToastComplete(self));
    } else {
      hideToastComplete(self);
    }
  };
  const showToast = (self) => {
    const { element, options } = self;
    shorty.Timer.set(
      element,
      () => {
        shorty.removeClass(element, hideClass);
        shorty.reflow(element);
        shorty.addClass(element, showClass);
        shorty.addClass(element, showingClass);
        if (options.animation) {
          shorty.emulateTransitionEnd(element, () => showToastComplete(self));
        } else {
          showToastComplete(self);
        }
      },
      17,
      showingClass
    );
  };
  const toggleToastHandlers = (self, add) => {
    const action = add ? eventListener$1.addListener : eventListener$1.removeListener;
    const { element, triggers, dismiss, options, hide } = self;
    if (dismiss) {
      action(dismiss, shorty.mouseclickEvent, hide);
    }
    if (options.autohide) {
      [shorty.focusinEvent, shorty.focusoutEvent, shorty.mouseenterEvent, shorty.mouseleaveEvent].forEach(
        (e) => action(element, e, interactiveToastHandler)
      );
    }
    if (triggers.length) {
      triggers.forEach((btn) => action(btn, shorty.mouseclickEvent, toastClickHandler));
    }
  };
  const completeDisposeToast = (self) => {
    shorty.Timer.clear(self.element, toastString);
    toggleToastHandlers(self);
  };
  const toastClickHandler = (e) => {
    const { target } = e;
    const trigger = target && shorty.closest(target, toastToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getToastInstance(element);
    if (!self)
      return;
    if (trigger && trigger.tagName === "A")
      e.preventDefault();
    self.relatedTarget = trigger;
    self.show();
  };
  const interactiveToastHandler = (e) => {
    const element = e.target;
    const self = getToastInstance(element);
    const { type, relatedTarget } = e;
    if (!self || element === relatedTarget || element.contains(relatedTarget))
      return;
    if ([shorty.mouseenterEvent, shorty.focusinEvent].includes(type)) {
      shorty.Timer.clear(element, toastString);
    } else {
      shorty.Timer.set(element, () => self.hide(), self.options.delay, toastString);
    }
  };
  class Toast extends BaseComponent {
    /**
     * @param target the target `.toast` element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      // TOAST PUBLIC METHODS
      // ====================
      /** Shows the toast. */
      __publicField(this, "show", () => {
        const { element, isShown } = this;
        if (element && !isShown) {
          shorty.dispatchEvent(element, showToastEvent);
          if (showToastEvent.defaultPrevented)
            return;
          showToast(this);
        }
      });
      /** Hides the toast. */
      __publicField(this, "hide", () => {
        const { element, isShown } = this;
        if (element && isShown) {
          shorty.dispatchEvent(element, hideToastEvent);
          if (hideToastEvent.defaultPrevented)
            return;
          hideToast(this);
        }
      });
      const { element, options } = this;
      if (options.animation && !shorty.hasClass(element, fadeClass))
        shorty.addClass(element, fadeClass);
      else if (!options.animation && shorty.hasClass(element, fadeClass))
        shorty.removeClass(element, fadeClass);
      this.dismiss = shorty.querySelector(toastDismissSelector, element);
      this.triggers = [...shorty.querySelectorAll(toastToggleSelector, shorty.getDocument(element))].filter(
        (btn) => getTargetElement(btn) === element
      );
      toggleToastHandlers(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return toastComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return toastDefaults;
    }
    /**
     * Returns *true* when toast is visible.
     */
    get isShown() {
      return shorty.hasClass(this.element, showClass);
    }
    /** Removes the `Toast` component from the target element. */
    dispose() {
      const { element, isShown } = this;
      if (isShown) {
        shorty.removeClass(element, showClass);
      }
      completeDisposeToast(this);
      super.dispose();
    }
  }
  __publicField(Toast, "selector", toastSelector);
  __publicField(Toast, "init", toastInitCallback);
  __publicField(Toast, "getInstance", getToastInstance);
  const componentsList = {
    Alert,
    Button,
    Carousel,
    Collapse,
    Dropdown,
    Modal,
    Offcanvas,
    Popover,
    Tab,
    Toast
  };
  const initComponentDataAPI = (callback, collection) => {
    [...collection].forEach((x) => callback(x));
  };
  const removeComponentDataAPI = (component, context) => {
    const compData = shorty.Data.getAllFor(component);
    if (compData) {
      [...compData].forEach(([element, instance]) => {
        if (context.contains(element))
          instance.dispose();
      });
    }
  };
  const initCallback = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    const elemCollection = [...shorty.getElementsByTagName("*", lookUp)];
    shorty.ObjectValues(componentsList).forEach((cs) => {
      const { init, selector } = cs;
      initComponentDataAPI(
        init,
        elemCollection.filter((item) => shorty.matches(item, selector))
      );
    });
  };
  const removeDataAPI = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    shorty.ObjectKeys(componentsList).forEach((comp) => {
      removeComponentDataAPI(comp, lookUp);
    });
  };
  if (document.body)
    initCallback();
  else {
    eventListener$1.addListener(document, "DOMContentLoaded", () => initCallback(), { once: true });
  }
  exports.Alert = Alert;
  exports.Button = Button;
  exports.Carousel = Carousel;
  exports.Collapse = Collapse;
  exports.Dropdown = Dropdown;
  exports.Listener = eventListener;
  exports.Modal = Modal;
  exports.Offcanvas = Offcanvas;
  exports.Popover = Popover;
  exports.Tab = Tab;
  exports.Toast = Toast;
  exports.initCallback = initCallback;
  exports.removeDataAPI = removeDataAPI;
  Object.defineProperty(exports, Symbol.toStringTag, { value: "Module" });
  return exports;
}({});
