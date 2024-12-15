var BSN = function(exports) {
  "use strict";
  const Me = "aria-describedby", De = "aria-expanded", $ = "aria-hidden", ze = "aria-modal", Ie = "aria-pressed", Pe = "aria-selected", st = "focus", rt = "focusin", ct = "focusout", lt = "keydown", ft = "keyup", gt = "click", vt = "mousedown", Et = "hover", ht = "mouseenter", yt = "mouseleave", Dt = "pointerdown", Ot = "pointermove", xt = "pointerup", Wt = "touchstart", Re = "dragstart", qt = 'a[href], button, input, textarea, select, details, [tabindex]:not([tabindex="-1"]', en = "ArrowDown", nn = "ArrowUp", on = "ArrowLeft", sn = "ArrowRight", fn = "Escape", _t = "transitionDuration", $t = "transitionDelay", M = "transitionend", W = "transitionProperty", On = () => {
    const t = /(iPhone|iPod|iPad)/;
    return navigator?.userAgentData?.brands.some(
      (e2) => t.test(e2.brand)
    ) || t.test(
      navigator?.userAgent
    ) || false;
  }, Yt = () => {
  }, R = (t, e2, n, o) => {
    const s = o || false;
    t.addEventListener(
      e2,
      n,
      s
    );
  }, Q = (t, e2, n, o) => {
    const s = o || false;
    t.removeEventListener(
      e2,
      n,
      s
    );
  }, j = (t, e2) => t.getAttribute(e2), ee = (t, e2) => t.hasAttribute(e2), Wn = (t, e2, n) => t.setAttribute(e2, n), Qn = (t, e2) => t.removeAttribute(e2), Kn = (t, ...e2) => {
    t.classList.add(...e2);
  }, qn = (t, ...e2) => {
    t.classList.remove(...e2);
  }, Gn = (t, e2) => t.classList.contains(e2), v$1 = (t) => t != null && typeof t == "object" || false, u = (t) => v$1(t) && typeof t.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some(
    (e2) => t.nodeType === e2
  ) || false, i = (t) => u(t) && t.nodeType === 1 || false, E$1 = /* @__PURE__ */ new Map(), L = {
    data: E$1,
    set: (t, e2, n) => {
      if (!i(t)) return;
      E$1.has(e2) || E$1.set(e2, /* @__PURE__ */ new Map()), E$1.get(e2).set(t, n);
    },
    getAllFor: (t) => E$1.get(t) || null,
    get: (t, e2) => {
      if (!i(t) || !e2) return null;
      const n = L.getAllFor(e2);
      return t && n && n.get(t) || null;
    },
    remove: (t, e2) => {
      const n = L.getAllFor(e2);
      !n || !i(t) || (n.delete(t), n.size === 0 && E$1.delete(e2));
    }
  }, Xn = (t, e2) => L.get(t, e2), I = (t) => t?.trim().replace(
    /(?:^\w|[A-Z]|\b\w)/g,
    (e2, n) => n === 0 ? e2.toLowerCase() : e2.toUpperCase()
  ).replace(/\s+/g, ""), N = (t) => typeof t == "string" || false, K = (t) => v$1(t) && t.constructor.name === "Window" || false, q = (t) => u(t) && t.nodeType === 9 || false, d = (t) => q(t) ? t : u(t) ? t.ownerDocument : K(t) ? t.document : globalThis.document, C = (t, ...e2) => Object.assign(t, ...e2), ne = (t) => {
    if (!t) return;
    if (N(t))
      return d().createElement(t);
    const { tagName: e2 } = t, n = ne(e2);
    if (!n) return;
    const o = { ...t };
    return delete o.tagName, C(n, o);
  }, G = (t, e2) => t.dispatchEvent(e2), f$1 = (t, e2, n) => {
    const o = getComputedStyle(t, n), s = e2.replace("webkit", "Webkit").replace(/([A-Z])/g, "-$1").toLowerCase();
    return o.getPropertyValue(s);
  }, ce = (t) => {
    const e2 = f$1(t, W), n = f$1(t, $t), o = n.includes("ms") ? 1 : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : 0;
    return Number.isNaN(s) ? 0 : s;
  }, ae = (t) => {
    const e2 = f$1(t, W), n = f$1(t, _t), o = n.includes("ms") ? 1 : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : 0;
    return Number.isNaN(s) ? 0 : s;
  }, no = (t, e2) => {
    let n = 0;
    const o = new Event(M), s = ae(t), r2 = ce(t);
    if (s) {
      const a2 = (l) => {
        l.target === t && (e2.apply(t, [l]), t.removeEventListener(M, a2), n = 1);
      };
      t.addEventListener(M, a2), setTimeout(() => {
        n || G(t, o);
      }, s + r2 + 17);
    } else
      e2.apply(t, [o]);
  }, ro = (t, e2) => t.focus(e2), P = (t) => ["true", true].includes(t) ? true : ["false", false].includes(t) ? false : ["null", "", null, void 0].includes(t) ? null : t !== "" && !Number.isNaN(+t) ? +t : t, S = (t) => Object.entries(t), ao = (t, e2, n, o) => {
    if (!i(t)) return e2;
    const s = { ...n }, r2 = { ...t.dataset }, a2 = { ...e2 }, l = {}, p2 = "title";
    return S(r2).forEach(([c, g]) => {
      const A = typeof c == "string" && c.includes(o) ? I(c.replace(o, "")) : I(c);
      l[A] = P(g);
    }), S(s).forEach(([c, g]) => {
      s[c] = P(g);
    }), S(e2).forEach(([c, g]) => {
      c in s ? a2[c] = s[c] : c in l ? a2[c] = l[c] : a2[c] = c === p2 ? j(t, p2) : g;
    }), a2;
  }, uo = (t) => Object.keys(t), po = (t, e2) => {
    const n = new CustomEvent(t, {
      cancelable: true,
      bubbles: true
    });
    return v$1(e2) && C(n, e2), n;
  }, go = { passive: true }, mo = (t) => t.offsetHeight, vo = (t, e2) => {
    S(e2).forEach(([n, o]) => {
      if (o && N(n) && n.includes("--"))
        t.style.setProperty(n, o);
      else {
        const s = {};
        s[n] = o, C(t.style, s);
      }
    });
  }, O = (t) => v$1(t) && t.constructor.name === "Map" || false, ie = (t) => typeof t == "number" || false, m$1 = /* @__PURE__ */ new Map(), bo = {
    set: (t, e2, n, o) => {
      i(t) && (o && o.length ? (m$1.has(t) || m$1.set(t, /* @__PURE__ */ new Map()), m$1.get(t).set(o, setTimeout(e2, n))) : m$1.set(t, setTimeout(e2, n)));
    },
    get: (t, e2) => {
      if (!i(t)) return null;
      const n = m$1.get(t);
      return e2 && n && O(n) ? n.get(e2) || null : ie(n) ? n : null;
    },
    clear: (t, e2) => {
      if (!i(t)) return;
      const n = m$1.get(t);
      e2 && e2.length && O(n) ? (clearTimeout(n.get(e2)), n.delete(e2), n.size === 0 && m$1.delete(t)) : (clearTimeout(n), m$1.delete(t));
    }
  }, Eo = (t) => t.toLowerCase(), ue = (t, e2) => (u(e2) ? e2 : d()).querySelectorAll(t), x = /* @__PURE__ */ new Map();
  function le(t) {
    const { shiftKey: e2, code: n } = t, o = d(this), s = [
      ...ue(qt, this)
    ].filter(
      (l) => !ee(l, "disabled") && !j(l, $)
    );
    if (!s.length) return;
    const r2 = s[0], a2 = s[s.length - 1];
    n === "Tab" && (e2 && o.activeElement === r2 ? (a2.focus(), t.preventDefault()) : !e2 && o.activeElement === a2 && (r2.focus(), t.preventDefault()));
  }
  const de = (t) => x.has(t) === true, yo = (t) => {
    const e2 = de(t);
    (e2 ? Q : R)(t, "keydown", le), e2 ? x.delete(t) : x.set(t, true);
  }, b = (t) => i(t) && "offsetWidth" in t || false, y = (t, e2) => {
    const { width: n, height: o, top: s, right: r2, bottom: a2, left: l } = t.getBoundingClientRect();
    let p2 = 1, c = 1;
    if (e2 && b(t)) {
      const { offsetWidth: g, offsetHeight: A } = t;
      p2 = g > 0 ? Math.round(n) / g : 1, c = A > 0 ? Math.round(o) / A : 1;
    }
    return {
      width: n / p2,
      height: o / c,
      top: s / c,
      right: r2 / p2,
      bottom: a2 / c,
      left: l / p2,
      x: l / p2,
      y: s / c
    };
  }, wo = (t) => d(t).body, w$1 = (t) => d(t).documentElement, So = (t) => {
    const e2 = K(t), n = e2 ? t.scrollX : t.scrollLeft, o = e2 ? t.scrollY : t.scrollTop;
    return { x: n, y: o };
  }, pe = (t) => u(t) && t.constructor.name === "ShadowRoot" || false, k$1 = (t) => t.nodeName === "HTML" ? t : i(t) && t.assignedSlot || u(t) && t.parentNode || pe(t) && t.host || w$1(t), ge = (t) => t ? q(t) ? t.defaultView : u(t) ? t?.ownerDocument?.defaultView : t : window, me = (t) => u(t) && ["TABLE", "TD", "TH"].includes(t.nodeName) || false, ve = (t, e2) => t.matches(e2), he = (t) => {
    if (!b(t)) return false;
    const { width: e2, height: n } = y(t), { offsetWidth: o, offsetHeight: s } = t;
    return Math.round(e2) !== o || Math.round(n) !== s;
  }, No = (t, e2, n) => {
    const o = b(e2), s = y(
      t,
      o && he(e2)
    ), r2 = { x: 0, y: 0 };
    if (o) {
      const a2 = y(e2, true);
      r2.x = a2.x + e2.clientLeft, r2.y = a2.y + e2.clientTop;
    }
    return {
      x: s.left + n.x - r2.x,
      y: s.top + n.y - r2.y,
      width: s.width,
      height: s.height
    };
  };
  let B = 0, V = 0;
  const h$1 = /* @__PURE__ */ new Map(), ye = (t, e2) => {
    let n = e2 ? B : V;
    if (e2) {
      const o = ye(t), s = h$1.get(o) || /* @__PURE__ */ new Map();
      h$1.has(o) || h$1.set(o, s), O(s) && !s.has(e2) ? (s.set(e2, n), B += 1) : n = s.get(e2);
    } else {
      const o = t.id || t;
      h$1.has(o) ? n = h$1.get(o) : (h$1.set(o, n), V += 1);
    }
    return n;
  }, we = (t) => Array.isArray(t) || false, To = (t) => {
    if (!u(t)) return false;
    const { top: e2, bottom: n } = y(t), { clientHeight: o } = w$1(t);
    return e2 <= o && n >= 0;
  }, Lo = (t) => typeof t == "function" || false, Fo = (t) => v$1(t) && t.constructor.name === "NodeList" || false, Bo = (t) => w$1(t).dir === "rtl", Se = (t, e2) => !t || !e2 ? null : t.closest(e2) || Se(t.getRootNode().host, e2) || null, Ho = (t, e2) => i(t) ? t : (i(e2) ? e2 : d()).querySelector(t), ke = (t, e2) => (u(e2) ? e2 : d()).getElementsByTagName(
    t
  ), Ro = (t, e2) => (e2 && u(e2) ? e2 : d()).getElementsByClassName(
    t
  );
  const e = {}, f = (t) => {
    const { type: n, currentTarget: c } = t;
    e[n].forEach((a2, s) => {
      c === s && a2.forEach((o, i2) => {
        i2.apply(s, [t]), typeof o == "object" && o.once && r(s, n, i2, o);
      });
    });
  }, E = (t, n, c, a2) => {
    e[n] || (e[n] = /* @__PURE__ */ new Map());
    const s = e[n];
    s.has(t) || s.set(t, /* @__PURE__ */ new Map());
    const o = s.get(
      t
    ), { size: i2 } = o;
    o.set(c, a2), i2 || t.addEventListener(
      n,
      f,
      a2
    );
  }, r = (t, n, c, a2) => {
    const s = e[n], o = s && s.get(t), i2 = o && o.get(c), d2 = i2 !== void 0 ? i2 : a2;
    o && o.has(c) && o.delete(c), s && (!o || !o.size) && s.delete(t), (!s || !s.size) && delete e[n], (!o || !o.size) && t.removeEventListener(
      n,
      f,
      d2
    );
  };
  const fadeClass = "fade";
  const showClass = "show";
  const dataBsDismiss = "data-bs-dismiss";
  const alertString = "alert";
  const alertComponent = "Alert";
  const isDisabled = (target) => {
    return Gn(target, "disabled") || j(target, "disabled") === "true";
  };
  const version = "5.1.0";
  const Version = version;
  class BaseComponent {
    constructor(target, config) {
      let element;
      try {
        if (i(target)) {
          element = target;
        } else if (N(target)) {
          element = Ho(target);
          if (!element) throw Error(`"${target}" is not a valid selector.`);
        } else {
          throw Error(`your target is not an instance of HTMLElement.`);
        }
      } catch (e2) {
        throw Error(`${this.name} Error: ${e2.message}`);
      }
      const prevInstance = L.get(element, this.name);
      if (prevInstance) {
        prevInstance._toggleEventListeners();
      }
      this.element = element;
      this.options = this.defaults && uo(this.defaults).length ? ao(element, this.defaults, config || {}, "bs") : {};
      L.set(element, this.name, this);
    }
    get version() {
      return Version;
    }
    get name() {
      return "BaseComponent";
    }
    get defaults() {
      return {};
    }
    _toggleEventListeners = () => {
    };
    dispose() {
      L.remove(this.element, this.name);
      uo(this).forEach((prop) => {
        delete this[prop];
      });
    }
  }
  const alertSelector = `.${alertString}`;
  const alertDismissSelector = `[${dataBsDismiss}="${alertString}"]`;
  const getAlertInstance = (element) => Xn(element, alertComponent);
  const alertInitCallback = (element) => new Alert(element);
  const closeAlertEvent = po(
    `close.bs.${alertString}`
  );
  const closedAlertEvent = po(
    `closed.bs.${alertString}`
  );
  const alertTransitionEnd = (self) => {
    const { element } = self;
    G(element, closedAlertEvent);
    self._toggleEventListeners();
    self.dispose();
    element.remove();
  };
  class Alert extends BaseComponent {
    static selector = alertSelector;
    static init = alertInitCallback;
    static getInstance = getAlertInstance;
    dismiss;
    constructor(target) {
      super(target);
      this.dismiss = Ho(
        alertDismissSelector,
        this.element
      );
      this._toggleEventListeners(true);
    }
    get name() {
      return alertComponent;
    }
    close = (e2) => {
      const { element, dismiss } = this;
      if (!element || !Gn(element, showClass)) return;
      if (e2 && dismiss && isDisabled(dismiss)) return;
      G(element, closeAlertEvent);
      if (closeAlertEvent.defaultPrevented) return;
      qn(element, showClass);
      if (Gn(element, fadeClass)) {
        no(element, () => alertTransitionEnd(this));
      } else alertTransitionEnd(this);
    };
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      const { dismiss, close } = this;
      if (dismiss) {
        action(dismiss, gt, close);
      }
    };
    dispose() {
      this._toggleEventListeners();
      super.dispose();
    }
  }
  const activeClass = "active";
  const dataBsToggle = "data-bs-toggle";
  const buttonString = "button";
  const buttonComponent = "Button";
  const buttonSelector = `[${dataBsToggle}="${buttonString}"]`;
  const getButtonInstance = (element) => Xn(element, buttonComponent);
  const buttonInitCallback = (element) => new Button(element);
  class Button extends BaseComponent {
    static selector = buttonSelector;
    static init = buttonInitCallback;
    static getInstance = getButtonInstance;
    constructor(target) {
      super(target);
      const { element } = this;
      this.isActive = Gn(element, activeClass);
      Wn(element, Ie, String(!!this.isActive));
      this._toggleEventListeners(true);
    }
    get name() {
      return buttonComponent;
    }
    toggle = (e2) => {
      if (e2) e2.preventDefault();
      const { element, isActive } = this;
      if (isDisabled(element)) return;
      const action = isActive ? qn : Kn;
      action(element, activeClass);
      Wn(element, Ie, isActive ? "false" : "true");
      this.isActive = Gn(element, activeClass);
    };
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      action(this.element, gt, this.toggle);
    };
    dispose() {
      this._toggleEventListeners();
      super.dispose();
    }
  }
  const dataBsTarget = "data-bs-target";
  const carouselString = "carousel";
  const carouselComponent = "Carousel";
  const dataBsParent = "data-bs-parent";
  const dataBsContainer = "data-bs-container";
  const getTargetElement = (element) => {
    const targetAttr = [dataBsTarget, dataBsParent, dataBsContainer, "href"];
    const doc = d(element);
    return targetAttr.map((att) => {
      const attValue = j(element, att);
      if (attValue) {
        return att === dataBsParent ? Se(element, attValue) : Ho(attValue, doc);
      }
      return null;
    }).filter((x2) => x2)[0];
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
  const getCarouselInstance = (element) => Xn(element, carouselComponent);
  const carouselInitCallback = (element) => new Carousel(element);
  let startX = 0;
  let currentX = 0;
  let endX = 0;
  const carouselSlideEvent = po(`slide.bs.${carouselString}`);
  const carouselSlidEvent = po(`slid.bs.${carouselString}`);
  const carouselTransitionEndHandler = (self) => {
    const { index, direction, element, slides, options } = self;
    if (self.isAnimating) {
      const activeItem = getActiveIndex(self);
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      Kn(slides[index], activeClass);
      qn(slides[index], `${carouselItem}-${orientation}`);
      qn(slides[index], `${carouselItem}-${directionClass}`);
      qn(slides[activeItem], activeClass);
      qn(slides[activeItem], `${carouselItem}-${directionClass}`);
      G(element, carouselSlidEvent);
      bo.clear(element, dataBsSlide);
      if (self.cycle && !d(element).hidden && options.interval && !self.isPaused) {
        self.cycle();
      }
    }
  };
  function carouselPauseHandler() {
    const self = getCarouselInstance(this);
    if (self && !self.isPaused && !bo.get(this, pausedClass)) {
      Kn(this, pausedClass);
    }
  }
  function carouselResumeHandler() {
    const self = getCarouselInstance(this);
    if (self && self.isPaused && !bo.get(this, pausedClass)) {
      self.cycle();
    }
  }
  function carouselIndicatorHandler(e2) {
    e2.preventDefault();
    const element = Se(this, carouselSelector) || getTargetElement(this);
    const self = element && getCarouselInstance(element);
    if (isDisabled(this)) return;
    if (!self || self.isAnimating) return;
    const newIndex = +(j(this, dataBsSlideTo) || 0);
    if (this && !Gn(this, activeClass) && !Number.isNaN(newIndex)) {
      self.to(newIndex);
    }
  }
  function carouselControlsHandler(e2) {
    e2.preventDefault();
    const element = Se(this, carouselSelector) || getTargetElement(this);
    const self = element && getCarouselInstance(element);
    if (isDisabled(this)) return;
    if (!self || self.isAnimating) return;
    const orientation = j(this, dataBsSlide);
    if (orientation === "next") {
      self.next();
    } else if (orientation === "prev") {
      self.prev();
    }
  }
  const carouselKeyHandler = ({ code, target }) => {
    const doc = d(target);
    const [element] = [...ue(carouselSelector, doc)].filter((x2) => To(x2));
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating || /textarea|input|select/i.test(target.nodeName)) return;
    const RTL = Bo(element);
    const arrowKeyNext = !RTL ? sn : on;
    const arrowKeyPrev = !RTL ? on : sn;
    if (code === arrowKeyPrev) self.prev();
    else if (code === arrowKeyNext) self.next();
  };
  function carouselDragHandler(e2) {
    const { target } = e2;
    const self = getCarouselInstance(this);
    if (self && self.isTouch && (self.indicator && !self.indicator.contains(target) || !self.controls.includes(target))) {
      e2.stopImmediatePropagation();
      e2.stopPropagation();
      e2.preventDefault();
    }
  }
  function carouselPointerDownHandler(e2) {
    const { target } = e2;
    const self = getCarouselInstance(this);
    if (!self || self.isAnimating || self.isTouch) return;
    const { controls, indicators } = self;
    if (![...controls, ...indicators].every(
      (el) => el === target || el.contains(target)
    )) {
      startX = e2.pageX;
      if (this.contains(target)) {
        self.isTouch = true;
        toggleCarouselTouchHandlers(self, true);
      }
    }
  }
  const carouselPointerMoveHandler = (e2) => {
    currentX = e2.pageX;
  };
  const carouselPointerUpHandler = (e2) => {
    const { target } = e2;
    const doc = d(target);
    const self = [...ue(carouselSelector, doc)].map((c) => getCarouselInstance(c)).find((i2) => i2.isTouch);
    if (!self) return;
    const { element, index } = self;
    const RTL = Bo(element);
    endX = e2.pageX;
    self.isTouch = false;
    toggleCarouselTouchHandlers(self);
    if (!doc.getSelection()?.toString().length && element.contains(target) && Math.abs(startX - endX) > 120) {
      if (currentX < startX) {
        self.to(index + (RTL ? -1 : 1));
      } else if (currentX > startX) {
        self.to(index + (RTL ? 1 : -1));
      }
    }
    startX = 0;
    currentX = 0;
    endX = 0;
  };
  const activateCarouselIndicator = (self, index) => {
    const { indicators } = self;
    [...indicators].forEach((x2) => qn(x2, activeClass));
    if (self.indicators[index]) Kn(indicators[index], activeClass);
  };
  const toggleCarouselTouchHandlers = (self, add) => {
    const { element } = self;
    const action = add ? E : r;
    action(
      d(element),
      Ot,
      carouselPointerMoveHandler,
      go
    );
    action(
      d(element),
      xt,
      carouselPointerUpHandler,
      go
    );
  };
  const getActiveIndex = (self) => {
    const { slides, element } = self;
    const activeItem = Ho(
      `.${carouselItem}.${activeClass}`,
      element
    );
    return activeItem ? [...slides].indexOf(activeItem) : -1;
  };
  class Carousel extends BaseComponent {
    static selector = carouselSelector;
    static init = carouselInitCallback;
    static getInstance = getCarouselInstance;
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.direction = Bo(element) ? "right" : "left";
      this.isTouch = false;
      this.slides = Ro(carouselItem, element);
      const { slides } = this;
      if (slides.length < 2) return;
      const activeIndex = getActiveIndex(this);
      const transitionItem = [...slides].find(
        (s) => ve(s, `.${carouselItem}-next`)
      );
      this.index = activeIndex;
      const doc = d(element);
      this.controls = [
        ...ue(`[${dataBsSlide}]`, element),
        ...ue(
          `[${dataBsSlide}][${dataBsTarget}="#${element.id}"]`,
          doc
        )
      ].filter((c, i2, ar) => i2 === ar.indexOf(c));
      this.indicator = Ho(
        `.${carouselString}-indicators`,
        element
      );
      this.indicators = [
        ...this.indicator ? ue(`[${dataBsSlideTo}]`, this.indicator) : [],
        ...ue(
          `[${dataBsSlideTo}][${dataBsTarget}="#${element.id}"]`,
          doc
        )
      ].filter((c, i2, ar) => i2 === ar.indexOf(c));
      const { options } = this;
      this.options.interval = options.interval === true ? carouselDefaults.interval : options.interval;
      if (transitionItem) {
        this.index = [...slides].indexOf(transitionItem);
      } else if (activeIndex < 0) {
        this.index = 0;
        Kn(slides[0], activeClass);
        if (this.indicators.length) activateCarouselIndicator(this, 0);
      }
      if (this.indicators.length) activateCarouselIndicator(this, this.index);
      this._toggleEventListeners(true);
      if (options.interval) this.cycle();
    }
    get name() {
      return carouselComponent;
    }
    get defaults() {
      return carouselDefaults;
    }
    get isPaused() {
      return Gn(this.element, pausedClass);
    }
    get isAnimating() {
      return Ho(
        `.${carouselItem}-next,.${carouselItem}-prev`,
        this.element
      ) !== null;
    }
    cycle() {
      const { element, options, isPaused, index } = this;
      bo.clear(element, carouselString);
      if (isPaused) {
        bo.clear(element, pausedClass);
        qn(element, pausedClass);
      }
      bo.set(
        element,
        () => {
          if (this.element && !this.isPaused && !this.isTouch && To(element)) {
            this.to(index + 1);
          }
        },
        options.interval,
        carouselString
      );
    }
    pause() {
      const { element, options } = this;
      if (this.isPaused || !options.interval) return;
      Kn(element, pausedClass);
      bo.set(
        element,
        () => {
        },
        1,
        pausedClass
      );
    }
    next() {
      if (!this.isAnimating) {
        this.to(this.index + 1);
      }
    }
    prev() {
      if (!this.isAnimating) {
        this.to(this.index - 1);
      }
    }
    to(idx) {
      const { element, slides, options } = this;
      const activeItem = getActiveIndex(this);
      const RTL = Bo(element);
      let next = idx;
      if (this.isAnimating || activeItem === next || bo.get(element, dataBsSlide)) return;
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
      C(carouselSlideEvent, eventProperties);
      C(carouselSlidEvent, eventProperties);
      G(element, carouselSlideEvent);
      if (carouselSlideEvent.defaultPrevented) return;
      this.index = next;
      activateCarouselIndicator(this, next);
      if (ae(slides[next]) && Gn(element, "slide")) {
        bo.set(
          element,
          () => {
            Kn(slides[next], `${carouselItem}-${orientation}`);
            mo(slides[next]);
            Kn(slides[next], `${carouselItem}-${directionClass}`);
            Kn(slides[activeItem], `${carouselItem}-${directionClass}`);
            no(
              slides[next],
              () => this.slides && this.slides.length && carouselTransitionEndHandler(this)
            );
          },
          0,
          dataBsSlide
        );
      } else {
        Kn(slides[next], activeClass);
        qn(slides[activeItem], activeClass);
        bo.set(
          element,
          () => {
            bo.clear(element, dataBsSlide);
            if (element && options.interval && !this.isPaused) {
              this.cycle();
            }
            G(element, carouselSlidEvent);
          },
          0,
          dataBsSlide
        );
      }
    }
    _toggleEventListeners = (add) => {
      const { element, options, slides, controls, indicators } = this;
      const { touch, pause, interval, keyboard } = options;
      const action = add ? E : r;
      if (pause && interval) {
        action(element, ht, carouselPauseHandler);
        action(element, yt, carouselResumeHandler);
      }
      if (touch && slides.length > 2) {
        action(
          element,
          Dt,
          carouselPointerDownHandler,
          go
        );
        action(element, Wt, carouselDragHandler, { passive: false });
        action(element, Re, carouselDragHandler, { passive: false });
      }
      if (controls.length) {
        controls.forEach((arrow) => {
          action(arrow, gt, carouselControlsHandler);
        });
      }
      if (indicators.length) {
        indicators.forEach((indicator) => {
          action(indicator, gt, carouselIndicatorHandler);
        });
      }
      if (keyboard) {
        action(d(element), lt, carouselKeyHandler);
      }
    };
    dispose() {
      const { isAnimating } = this;
      const clone = {
        ...this,
        isAnimating
      };
      this._toggleEventListeners();
      super.dispose();
      if (clone.isAnimating) {
        no(clone.slides[clone.index], () => {
          carouselTransitionEndHandler(clone);
        });
      }
    }
  }
  const collapsingClass = "collapsing";
  const collapseString = "collapse";
  const collapseComponent = "Collapse";
  const collapseSelector = `.${collapseString}`;
  const collapseToggleSelector = `[${dataBsToggle}="${collapseString}"]`;
  const collapseDefaults = { parent: null };
  const getCollapseInstance = (element) => Xn(element, collapseComponent);
  const collapseInitCallback = (element) => new Collapse(element);
  const showCollapseEvent = po(`show.bs.${collapseString}`);
  const shownCollapseEvent = po(`shown.bs.${collapseString}`);
  const hideCollapseEvent = po(`hide.bs.${collapseString}`);
  const hiddenCollapseEvent = po(`hidden.bs.${collapseString}`);
  const expandCollapse = (self) => {
    const { element, parent, triggers } = self;
    G(element, showCollapseEvent);
    if (!showCollapseEvent.defaultPrevented) {
      bo.set(element, Yt, 17);
      if (parent) bo.set(parent, Yt, 17);
      Kn(element, collapsingClass);
      qn(element, collapseString);
      vo(element, { height: `${element.scrollHeight}px` });
      no(element, () => {
        bo.clear(element);
        if (parent) bo.clear(parent);
        triggers.forEach((btn) => Wn(btn, De, "true"));
        qn(element, collapsingClass);
        Kn(element, collapseString);
        Kn(element, showClass);
        vo(element, { height: "" });
        G(element, shownCollapseEvent);
      });
    }
  };
  const collapseContent = (self) => {
    const { element, parent, triggers } = self;
    G(element, hideCollapseEvent);
    if (!hideCollapseEvent.defaultPrevented) {
      bo.set(element, Yt, 17);
      if (parent) bo.set(parent, Yt, 17);
      vo(element, { height: `${element.scrollHeight}px` });
      qn(element, collapseString);
      qn(element, showClass);
      Kn(element, collapsingClass);
      mo(element);
      vo(element, { height: "0px" });
      no(element, () => {
        bo.clear(element);
        if (parent) bo.clear(parent);
        triggers.forEach((btn) => Wn(btn, De, "false"));
        qn(element, collapsingClass);
        Kn(element, collapseString);
        vo(element, { height: "" });
        G(element, hiddenCollapseEvent);
      });
    }
  };
  const collapseClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && Se(target, collapseToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getCollapseInstance(element);
    if (trigger && isDisabled(trigger)) return;
    if (!self) return;
    self.toggle();
    if (trigger?.tagName === "A") e2.preventDefault();
  };
  class Collapse extends BaseComponent {
    static selector = collapseSelector;
    static init = collapseInitCallback;
    static getInstance = getCollapseInstance;
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      const doc = d(element);
      this.triggers = [...ue(collapseToggleSelector, doc)].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.parent = b(options.parent) ? options.parent : N(options.parent) ? getTargetElement(element) || Ho(options.parent, doc) : null;
      this._toggleEventListeners(true);
    }
    get name() {
      return collapseComponent;
    }
    get defaults() {
      return collapseDefaults;
    }
    hide() {
      const { triggers, element } = this;
      if (!bo.get(element)) {
        collapseContent(this);
        if (triggers.length) {
          triggers.forEach((btn) => Kn(btn, `${collapseString}d`));
        }
      }
    }
    show() {
      const { element, parent, triggers } = this;
      let activeCollapse;
      let activeCollapseInstance;
      if (parent) {
        activeCollapse = [
          ...ue(`.${collapseString}.${showClass}`, parent)
        ].find((i2) => getCollapseInstance(i2));
        activeCollapseInstance = activeCollapse && getCollapseInstance(activeCollapse);
      }
      if ((!parent || !bo.get(parent)) && !bo.get(element)) {
        if (activeCollapseInstance && activeCollapse !== element) {
          collapseContent(activeCollapseInstance);
          activeCollapseInstance.triggers.forEach((btn) => {
            Kn(btn, `${collapseString}d`);
          });
        }
        expandCollapse(this);
        if (triggers.length) {
          triggers.forEach((btn) => qn(btn, `${collapseString}d`));
        }
      }
    }
    toggle() {
      if (!Gn(this.element, showClass)) this.show();
      else this.hide();
    }
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      const { triggers } = this;
      if (triggers.length) {
        triggers.forEach((btn) => {
          action(btn, gt, collapseClickHandler);
        });
      }
    };
    dispose() {
      this._toggleEventListeners();
      super.dispose();
    }
  }
  const m = (e2) => e2 != null && typeof e2 == "object" || false, p = (e2) => m(e2) && typeof e2.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some(
    (t) => e2.nodeType === t
  ) || false, h = (e2) => p(e2) && e2.nodeType === 1 || false, w = (e2) => typeof e2 == "function" || false, k = "1.0.2", a = "PositionObserver Error";
  class v {
    entries;
    static version = k;
    _tick;
    _root;
    _callback;
    /**
     * The constructor takes two arguments, a `callback`, which is called
     * whenever the position of an observed element changes and an `options` object.
     * The callback function should take an array of `PositionObserverEntry` objects
     * as its only argument, but it's not required.
     *
     * @param callback the callback that applies to all targets of this observer
     * @param options the options of this observer
     */
    constructor(t, i2) {
      if (!w(t))
        throw new Error(`${a}: ${t} is not a function.`);
      this.entries = /* @__PURE__ */ new Map(), this._callback = t, this._root = h(i2?.root) ? i2.root : document?.documentElement, this._tick = 0;
    }
    /**
     * Start observing the position of the specified element.
     * If the element is not currently attached to the DOM,
     * it will NOT be added to the entries.
     *
     * @param target an `Element` target
     */
    observe = (t) => {
      if (!h(t))
        throw new Error(
          `${a}: ${t} is not an instance of Element.`
        );
      this._root.contains(t) && this._new(t).then((i2) => {
        i2 && !this.getEntry(t) && this.entries.set(t, i2), this._tick || (this._tick = requestAnimationFrame(this._runCallback));
      });
    };
    /**
     * Stop observing the position of the specified element.
     *
     * @param target an `HTMLElement` target
     */
    unobserve = (t) => {
      this.entries.has(t) && this.entries.delete(t);
    };
    /**
     * Private method responsible for all the heavy duty,
     * the observer's runtime.
     */
    _runCallback = () => {
      if (!this.entries.size) return;
      const t = new Promise((i2) => {
        const r2 = [];
        this.entries.forEach(
          ({ target: s, boundingClientRect: n }) => {
            this._root.contains(s) && this._new(s).then(({ boundingClientRect: o, isIntersecting: u2 }) => {
              if (!u2) return;
              const { left: f2, top: _, bottom: l, right: b2 } = o;
              if (n.top !== _ || n.left !== f2 || n.right !== b2 || n.bottom !== l) {
                const c = { target: s, boundingClientRect: o };
                this.entries.set(s, c), r2.push(c);
              }
            });
          }
        ), i2(r2);
      });
      this._tick = requestAnimationFrame(async () => {
        const i2 = await t;
        i2.length && this._callback(i2, this), this._runCallback();
      });
    };
    /**
     * Calculate the target bounding box and determine
     * the value of `isVisible`.
     *
     * @param target an `Element` target
     */
    _new = (t) => new Promise((i2) => {
      new IntersectionObserver(
        ([s], n) => {
          n.disconnect(), i2(s);
        }
      ).observe(t);
    });
    /**
     * Find the entry for a given target.
     *
     * @param target an `HTMLElement` target
     */
    getEntry = (t) => this.entries.get(t);
    /**
     * Immediately stop observing all elements.
     */
    disconnect = () => {
      cancelAnimationFrame(this._tick), this.entries.clear(), this._tick = 0;
    };
  }
  const dropdownMenuClasses = ["dropdown", "dropup", "dropstart", "dropend"];
  const dropdownComponent = "Dropdown";
  const dropdownMenuClass = "dropdown-menu";
  const isEmptyAnchor = (element) => {
    const parentAnchor = Se(element, "A");
    return element.tagName === "A" && ee(element, "href") && j(element, "href")?.slice(-1) === "#" || parentAnchor && ee(parentAnchor, "href") && j(parentAnchor, "href")?.slice(-1) === "#";
  };
  const [dropdownString, dropupString, dropstartString, dropendString] = dropdownMenuClasses;
  const dropdownSelector = `[${dataBsToggle}="${dropdownString}"]`;
  const getDropdownInstance = (element) => Xn(element, dropdownComponent);
  const dropdownInitCallback = (element) => new Dropdown(element);
  const dropdownMenuEndClass = `${dropdownMenuClass}-end`;
  const verticalClass = [dropdownString, dropupString];
  const horizontalClass = [dropstartString, dropendString];
  const menuFocusTags = ["A", "BUTTON"];
  const dropdownDefaults = {
    offset: 5,
    display: "dynamic"
  };
  const showDropdownEvent = po(
    `show.bs.${dropdownString}`
  );
  const shownDropdownEvent = po(
    `shown.bs.${dropdownString}`
  );
  const hideDropdownEvent = po(
    `hide.bs.${dropdownString}`
  );
  const hiddenDropdownEvent = po(`hidden.bs.${dropdownString}`);
  const updatedDropdownEvent = po(`updated.bs.${dropdownString}`);
  const styleDropdown = (self) => {
    const { element, menu, parentElement, options } = self;
    const { offset } = options;
    if (f$1(menu, "position") === "static") return;
    const RTL = Bo(element);
    const menuEnd = Gn(menu, dropdownMenuEndClass);
    const resetProps = ["margin", "top", "bottom", "left", "right"];
    resetProps.forEach((p2) => {
      const style = {};
      style[p2] = "";
      vo(menu, style);
    });
    let positionClass = dropdownMenuClasses.find((c) => Gn(parentElement, c)) || dropdownString;
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
    const { clientWidth, clientHeight } = w$1(element);
    const {
      left: targetLeft,
      top: targetTop,
      width: targetWidth,
      height: targetHeight
    } = y(element);
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
      C(dropdownPosition[positionClass], {
        top: "auto",
        bottom: 0
      });
    }
    if (verticalClass.includes(positionClass) && (leftExceed || rightExceed)) {
      let posAjust = { left: "auto", right: "auto" };
      if (!leftExceed && rightExceed && !RTL) {
        posAjust = { left: "auto", right: 0 };
      }
      if (leftExceed && !rightExceed && RTL) {
        posAjust = { left: 0, right: "auto" };
      }
      if (posAjust) {
        C(dropdownPosition[positionClass], posAjust);
      }
    }
    const margins = dropdownMargin[positionClass];
    vo(menu, {
      ...dropdownPosition[positionClass],
      margin: `${margins.map((x2) => x2 ? `${x2}px` : x2).join(" ")}`
    });
    if (verticalClass.includes(positionClass) && menuEnd) {
      if (menuEnd) {
        const endAdjust = !RTL && leftExceed || RTL && rightExceed ? "menuStart" : "menuEnd";
        vo(menu, dropdownPosition[endAdjust]);
      }
    }
    G(parentElement, updatedDropdownEvent);
  };
  const getMenuItems = (menu) => {
    return Array.from(menu.children).map((c) => {
      if (c && menuFocusTags.includes(c.tagName)) return c;
      const { firstElementChild } = c;
      if (firstElementChild && menuFocusTags.includes(firstElementChild.tagName)) {
        return firstElementChild;
      }
      return null;
    }).filter((c) => c);
  };
  const toggleDropdownDismiss = (self) => {
    const { element, options, menu } = self;
    const action = self.open ? E : r;
    const doc = d(element);
    action(doc, gt, dropdownDismissHandler);
    action(doc, st, dropdownDismissHandler);
    action(doc, lt, dropdownPreventScroll);
    action(doc, ft, dropdownKeyHandler);
    if (options.display === "dynamic") {
      if (self.open) self._observer.observe(menu);
      else self._observer.disconnect();
    }
  };
  const getCurrentOpenDropdown = (element) => {
    const currentParent = [...dropdownMenuClasses, "btn-group", "input-group"].map(
      (c) => Ro(`${c} ${showClass}`, d(element))
    ).find((x2) => x2.length);
    if (currentParent && currentParent.length) {
      return [...currentParent[0].children].find(
        (x2) => dropdownMenuClasses.some((c) => c === j(x2, dataBsToggle))
      );
    }
    return void 0;
  };
  const dropdownDismissHandler = (e2) => {
    const { target, type } = e2;
    if (!b(target)) return;
    const element = getCurrentOpenDropdown(target);
    const self = element && getDropdownInstance(element);
    if (!self) return;
    const { parentElement, menu } = self;
    const isForm = parentElement && parentElement.contains(target) && (target.tagName === "form" || Se(target, "form") !== null);
    if ([gt, vt].includes(type) && isEmptyAnchor(target)) {
      e2.preventDefault();
    }
    if (!isForm && type !== st && target !== element && target !== menu) {
      self.hide();
    }
  };
  function dropdownClickHandler(e2) {
    const self = getDropdownInstance(this);
    if (isDisabled(this)) return;
    if (!self) return;
    e2.stopPropagation();
    self.toggle();
    if (isEmptyAnchor(this)) e2.preventDefault();
  }
  const dropdownPreventScroll = (e2) => {
    if ([en, nn].includes(e2.code)) e2.preventDefault();
  };
  function dropdownKeyHandler(e2) {
    const { code } = e2;
    const element = getCurrentOpenDropdown(this);
    if (!element) return;
    const self = getDropdownInstance(element);
    const { activeElement } = d(element);
    if (!self || !activeElement) return;
    const { menu, open } = self;
    const menuItems = getMenuItems(menu);
    if (menuItems && menuItems.length && [en, nn].includes(code)) {
      let idx = menuItems.indexOf(activeElement);
      if (activeElement === element) {
        idx = 0;
      } else if (code === nn) {
        idx = idx > 1 ? idx - 1 : 0;
      } else if (code === en) {
        idx = idx < menuItems.length - 1 ? idx + 1 : idx;
      }
      if (menuItems[idx]) ro(menuItems[idx]);
    }
    if (fn === code && open) {
      self.toggle();
      ro(element);
    }
  }
  class Dropdown extends BaseComponent {
    static selector = dropdownSelector;
    static init = dropdownInitCallback;
    static getInstance = getDropdownInstance;
    constructor(target, config) {
      super(target, config);
      const { parentElement } = this.element;
      const [menu] = Ro(
        dropdownMenuClass,
        parentElement
      );
      if (!menu) return;
      this.parentElement = parentElement;
      this.menu = menu;
      this._observer = new v(
        () => styleDropdown(this)
      );
      this._toggleEventListeners(true);
    }
    get name() {
      return dropdownComponent;
    }
    get defaults() {
      return dropdownDefaults;
    }
    toggle() {
      if (this.open) this.hide();
      else this.show();
    }
    show() {
      const { element, open, menu, parentElement } = this;
      if (open) return;
      const currentElement = getCurrentOpenDropdown(element);
      const currentInstance = currentElement && getDropdownInstance(currentElement);
      if (currentInstance) currentInstance.hide();
      [showDropdownEvent, shownDropdownEvent, updatedDropdownEvent].forEach(
        (e2) => {
          e2.relatedTarget = element;
        }
      );
      G(parentElement, showDropdownEvent);
      if (showDropdownEvent.defaultPrevented) return;
      Kn(menu, showClass);
      Kn(parentElement, showClass);
      Wn(element, De, "true");
      styleDropdown(this);
      this.open = !open;
      ro(element);
      toggleDropdownDismiss(this);
      G(parentElement, shownDropdownEvent);
    }
    hide() {
      const { element, open, menu, parentElement } = this;
      if (!open) return;
      [hideDropdownEvent, hiddenDropdownEvent].forEach((e2) => {
        e2.relatedTarget = element;
      });
      G(parentElement, hideDropdownEvent);
      if (hideDropdownEvent.defaultPrevented) return;
      qn(menu, showClass);
      qn(parentElement, showClass);
      Wn(element, De, "false");
      this.open = !open;
      toggleDropdownDismiss(this);
      G(parentElement, hiddenDropdownEvent);
    }
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      action(this.element, gt, dropdownClickHandler);
    };
    dispose() {
      if (this.open) this.hide();
      this._toggleEventListeners();
      super.dispose();
    }
  }
  const modalString = "modal";
  const modalComponent = "Modal";
  const offcanvasComponent = "Offcanvas";
  const fixedTopClass = "fixed-top";
  const fixedBottomClass = "fixed-bottom";
  const stickyTopClass = "sticky-top";
  const positionStickyClass = "position-sticky";
  const getFixedItems = (parent) => [
    ...Ro(fixedTopClass, parent),
    ...Ro(fixedBottomClass, parent),
    ...Ro(stickyTopClass, parent),
    ...Ro(positionStickyClass, parent),
    ...Ro("is-fixed", parent)
  ];
  const resetScrollbar = (element) => {
    const bd = wo(element);
    vo(bd, {
      paddingRight: "",
      overflow: ""
    });
    const fixedItems = getFixedItems(bd);
    if (fixedItems.length) {
      fixedItems.forEach((fixed) => {
        vo(fixed, {
          paddingRight: "",
          marginRight: ""
        });
      });
    }
  };
  const measureScrollbar = (element) => {
    const { clientWidth } = w$1(element);
    const { innerWidth } = ge(element);
    return Math.abs(innerWidth - clientWidth);
  };
  const setScrollbar = (element, overflow) => {
    const bd = wo(element);
    const bodyPad = parseInt(f$1(bd, "paddingRight"), 10);
    const isOpen = f$1(bd, "overflow") === "hidden";
    const sbWidth = isOpen && bodyPad ? 0 : measureScrollbar(element);
    const fixedItems = getFixedItems(bd);
    if (!overflow) return;
    vo(bd, {
      overflow: "hidden",
      paddingRight: `${bodyPad + sbWidth}px`
    });
    if (!fixedItems.length) return;
    fixedItems.forEach((fixed) => {
      const itemPadValue = f$1(fixed, "paddingRight");
      fixed.style.paddingRight = `${parseInt(itemPadValue, 10) + sbWidth}px`;
      if ([stickyTopClass, positionStickyClass].some((c) => Gn(fixed, c))) {
        const itemMValue = f$1(fixed, "marginRight");
        fixed.style.marginRight = `${parseInt(itemMValue, 10) - sbWidth}px`;
      }
    });
  };
  const offcanvasString = "offcanvas";
  const popupContainer = ne({
    tagName: "div",
    className: "popup-container"
  });
  const appendPopup = (target, customContainer) => {
    const containerIsBody = u(customContainer) && customContainer.nodeName === "BODY";
    const lookup = u(customContainer) && !containerIsBody ? customContainer : popupContainer;
    const BODY = containerIsBody ? customContainer : wo(target);
    if (u(target)) {
      if (lookup === popupContainer) {
        BODY.append(popupContainer);
      }
      lookup.append(target);
    }
  };
  const removePopup = (target, customContainer) => {
    const containerIsBody = u(customContainer) && customContainer.nodeName === "BODY";
    const lookup = u(customContainer) && !containerIsBody ? customContainer : popupContainer;
    if (u(target)) {
      target.remove();
      if (lookup === popupContainer && !popupContainer.children.length) {
        popupContainer.remove();
      }
    }
  };
  const hasPopup = (target, customContainer) => {
    const lookup = u(customContainer) && customContainer.nodeName !== "BODY" ? customContainer : popupContainer;
    return u(target) && lookup.contains(target);
  };
  const backdropString = "backdrop";
  const modalBackdropClass = `${modalString}-${backdropString}`;
  const offcanvasBackdropClass = `${offcanvasString}-${backdropString}`;
  const modalActiveSelector = `.${modalString}.${showClass}`;
  const offcanvasActiveSelector = `.${offcanvasString}.${showClass}`;
  const overlay = ne("div");
  const getCurrentOpen = (element) => {
    return Ho(
      `${modalActiveSelector},${offcanvasActiveSelector}`,
      d(element)
    );
  };
  const toggleOverlayType = (isModal) => {
    const targetClass = isModal ? modalBackdropClass : offcanvasBackdropClass;
    [modalBackdropClass, offcanvasBackdropClass].forEach((c) => {
      qn(overlay, c);
    });
    Kn(overlay, targetClass);
  };
  const appendOverlay = (element, hasFade, isModal) => {
    toggleOverlayType(isModal);
    appendPopup(overlay, wo(element));
    if (hasFade) Kn(overlay, fadeClass);
  };
  const showOverlay = () => {
    if (!Gn(overlay, showClass)) {
      Kn(overlay, showClass);
      mo(overlay);
    }
  };
  const hideOverlay = () => {
    qn(overlay, showClass);
  };
  const removeOverlay = (element) => {
    if (!getCurrentOpen(element)) {
      qn(overlay, fadeClass);
      removePopup(overlay, wo(element));
      resetScrollbar(element);
    }
  };
  const isVisible = (element) => {
    return b(element) && f$1(element, "visibility") !== "hidden" && element.offsetParent !== null;
  };
  const modalSelector = `.${modalString}`;
  const modalToggleSelector = `[${dataBsToggle}="${modalString}"]`;
  const modalDismissSelector = `[${dataBsDismiss}="${modalString}"]`;
  const modalStaticClass = `${modalString}-static`;
  const modalDefaults = {
    backdrop: true,
    keyboard: true
  };
  const getModalInstance = (element) => Xn(element, modalComponent);
  const modalInitCallback = (element) => new Modal(element);
  const showModalEvent = po(
    `show.bs.${modalString}`
  );
  const shownModalEvent = po(
    `shown.bs.${modalString}`
  );
  const hideModalEvent = po(
    `hide.bs.${modalString}`
  );
  const hiddenModalEvent = po(
    `hidden.bs.${modalString}`
  );
  const setModalScrollbar = (self) => {
    const { element } = self;
    const scrollbarWidth = measureScrollbar(element);
    const { clientHeight, scrollHeight } = w$1(element);
    const { clientHeight: modalHeight, scrollHeight: modalScrollHeight } = element;
    const modalOverflow = modalHeight !== modalScrollHeight;
    if (!modalOverflow && scrollbarWidth) {
      const pad = !Bo(element) ? "paddingRight" : "paddingLeft";
      const padStyle = { [pad]: `${scrollbarWidth}px` };
      vo(element, padStyle);
    }
    setScrollbar(element, modalOverflow || clientHeight !== scrollHeight);
  };
  const toggleModalDismiss = (self, add) => {
    const action = add ? E : r;
    const { element } = self;
    action(element, gt, modalDismissHandler);
    action(d(element), lt, modalKeyHandler);
    if (add) self._observer.observe(element);
    else self._observer.disconnect();
  };
  const afterModalHide = (self) => {
    const { triggers, element, relatedTarget } = self;
    removeOverlay(element);
    vo(element, { paddingRight: "", display: "" });
    toggleModalDismiss(self);
    const focusElement = showModalEvent.relatedTarget || triggers.find(isVisible);
    if (focusElement) ro(focusElement);
    hiddenModalEvent.relatedTarget = relatedTarget || void 0;
    G(element, hiddenModalEvent);
    yo(element);
  };
  const afterModalShow = (self) => {
    const { element, relatedTarget } = self;
    ro(element);
    toggleModalDismiss(self, true);
    shownModalEvent.relatedTarget = relatedTarget || void 0;
    G(element, shownModalEvent);
    yo(element);
  };
  const beforeModalShow = (self) => {
    const { element, hasFade } = self;
    vo(element, { display: "block" });
    setModalScrollbar(self);
    if (!getCurrentOpen(element)) {
      vo(wo(element), { overflow: "hidden" });
    }
    Kn(element, showClass);
    Qn(element, $);
    Wn(element, ze, "true");
    if (hasFade) no(element, () => afterModalShow(self));
    else afterModalShow(self);
  };
  const beforeModalHide = (self) => {
    const { element, options, hasFade } = self;
    if (options.backdrop && hasFade && Gn(overlay, showClass) && !getCurrentOpen(element)) {
      hideOverlay();
      no(overlay, () => afterModalHide(self));
    } else {
      afterModalHide(self);
    }
  };
  function modalClickHandler(e2) {
    const element = getTargetElement(this);
    const self = element && getModalInstance(element);
    if (isDisabled(this)) return;
    if (!self) return;
    if (this.tagName === "A") e2.preventDefault();
    self.relatedTarget = this;
    self.toggle();
  }
  const modalKeyHandler = ({ code, target }) => {
    const element = Ho(modalActiveSelector, d(target));
    const self = element && getModalInstance(element);
    if (!self) return;
    const { options } = self;
    if (options.keyboard && code === fn && Gn(element, showClass)) {
      self.relatedTarget = null;
      self.hide();
    }
  };
  const modalDismissHandler = (e2) => {
    const { currentTarget } = e2;
    const self = currentTarget && getModalInstance(currentTarget);
    if (!self || !currentTarget || bo.get(currentTarget)) return;
    const { options, isStatic, modalDialog } = self;
    const { backdrop } = options;
    const { target } = e2;
    const selectedText = d(currentTarget)?.getSelection()?.toString().length;
    const targetInsideDialog = modalDialog.contains(target);
    const dismiss = target && Se(target, modalDismissSelector);
    if (isStatic && !targetInsideDialog) {
      bo.set(
        currentTarget,
        () => {
          Kn(currentTarget, modalStaticClass);
          no(modalDialog, () => staticTransitionEnd(self));
        },
        17
      );
    } else if (dismiss || !selectedText && !isStatic && !targetInsideDialog && backdrop) {
      self.relatedTarget = dismiss || null;
      self.hide();
      e2.preventDefault();
    }
  };
  const staticTransitionEnd = (self) => {
    const { element, modalDialog } = self;
    const duration = (ae(modalDialog) || 0) + 17;
    qn(element, modalStaticClass);
    bo.set(element, () => bo.clear(element), duration);
  };
  class Modal extends BaseComponent {
    static selector = modalSelector;
    static init = modalInitCallback;
    static getInstance = getModalInstance;
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      const modalDialog = Ho(
        `.${modalString}-dialog`,
        element
      );
      if (!modalDialog) return;
      this.modalDialog = modalDialog;
      this.triggers = [
        ...ue(
          modalToggleSelector,
          d(element)
        )
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.isStatic = this.options.backdrop === "static";
      this.hasFade = Gn(element, fadeClass);
      this.relatedTarget = null;
      this._observer = new ResizeObserver(() => this.update());
      this._toggleEventListeners(true);
    }
    get name() {
      return modalComponent;
    }
    get defaults() {
      return modalDefaults;
    }
    toggle() {
      if (Gn(this.element, showClass)) this.hide();
      else this.show();
    }
    show() {
      const { element, options, hasFade, relatedTarget } = this;
      const { backdrop } = options;
      let overlayDelay = 0;
      if (Gn(element, showClass)) return;
      showModalEvent.relatedTarget = relatedTarget || void 0;
      G(element, showModalEvent);
      if (showModalEvent.defaultPrevented) return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getModalInstance(currentOpen) || Xn(
          currentOpen,
          offcanvasComponent
        );
        if (that) that.hide();
      }
      if (backdrop) {
        if (!hasPopup(overlay)) {
          appendOverlay(element, hasFade, true);
        } else {
          toggleOverlayType(true);
        }
        overlayDelay = ae(overlay);
        showOverlay();
        setTimeout(() => beforeModalShow(this), overlayDelay);
      } else {
        beforeModalShow(this);
        if (currentOpen && Gn(overlay, showClass)) {
          hideOverlay();
        }
      }
    }
    hide() {
      const { element, hasFade, relatedTarget } = this;
      if (!Gn(element, showClass)) return;
      hideModalEvent.relatedTarget = relatedTarget || void 0;
      G(element, hideModalEvent);
      if (hideModalEvent.defaultPrevented) return;
      qn(element, showClass);
      Wn(element, $, "true");
      Qn(element, ze);
      if (hasFade) no(element, () => beforeModalHide(this));
      else beforeModalHide(this);
    }
    update = () => {
      if (Gn(this.element, showClass)) setModalScrollbar(this);
    };
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      const { triggers } = this;
      if (!triggers.length) return;
      triggers.forEach((btn) => {
        action(btn, gt, modalClickHandler);
      });
    };
    dispose() {
      const clone = { ...this };
      const { modalDialog, hasFade } = clone;
      const callback = () => setTimeout(() => super.dispose(), 17);
      this.hide();
      this._toggleEventListeners();
      if (hasFade) {
        no(modalDialog, callback);
      } else {
        callback();
      }
    }
  }
  const offcanvasSelector = `.${offcanvasString}`;
  const offcanvasToggleSelector = `[${dataBsToggle}="${offcanvasString}"]`;
  const offcanvasDismissSelector = `[${dataBsDismiss}="${offcanvasString}"]`;
  const offcanvasTogglingClass = `${offcanvasString}-toggling`;
  const offcanvasDefaults = {
    backdrop: true,
    keyboard: true,
    scroll: false
  };
  const getOffcanvasInstance = (element) => Xn(element, offcanvasComponent);
  const offcanvasInitCallback = (element) => new Offcanvas(element);
  const showOffcanvasEvent = po(`show.bs.${offcanvasString}`);
  const shownOffcanvasEvent = po(`shown.bs.${offcanvasString}`);
  const hideOffcanvasEvent = po(`hide.bs.${offcanvasString}`);
  const hiddenOffcanvasEvent = po(`hidden.bs.${offcanvasString}`);
  const setOffCanvasScrollbar = (self) => {
    const { element } = self;
    const { clientHeight, scrollHeight } = w$1(element);
    setScrollbar(element, clientHeight !== scrollHeight);
  };
  const toggleOffCanvasDismiss = (self, add) => {
    const action = add ? E : r;
    const doc = d(self.element);
    action(doc, lt, offcanvasKeyDismissHandler);
    action(doc, gt, offcanvasDismissHandler);
  };
  const beforeOffcanvasShow = (self) => {
    const { element, options } = self;
    if (!options.scroll) {
      setOffCanvasScrollbar(self);
      vo(wo(element), { overflow: "hidden" });
    }
    Kn(element, offcanvasTogglingClass);
    Kn(element, showClass);
    vo(element, { visibility: "visible" });
    no(element, () => showOffcanvasComplete(self));
  };
  const beforeOffcanvasHide = (self) => {
    const { element, options } = self;
    const currentOpen = getCurrentOpen(element);
    element.blur();
    if (!currentOpen && options.backdrop && Gn(overlay, showClass)) {
      hideOverlay();
    }
    no(element, () => hideOffcanvasComplete(self));
  };
  function offcanvasTriggerHandler(e2) {
    const element = getTargetElement(this);
    const self = element && getOffcanvasInstance(element);
    if (isDisabled(this)) return;
    if (!self) return;
    self.relatedTarget = this;
    self.toggle();
    if (this.tagName === "A") e2.preventDefault();
  }
  const offcanvasDismissHandler = (e2) => {
    const { target } = e2;
    const element = Ho(
      offcanvasActiveSelector,
      d(target)
    );
    if (!element) return;
    const offCanvasDismiss = Ho(
      offcanvasDismissSelector,
      element
    );
    const self = getOffcanvasInstance(element);
    if (!self) return;
    const { options, triggers } = self;
    const { backdrop } = options;
    const trigger = Se(target, offcanvasToggleSelector);
    const selection = d(element).getSelection();
    if (overlay.contains(target) && backdrop === "static") return;
    if (!(selection && selection.toString().length) && (!element.contains(target) && backdrop && (!trigger || triggers.includes(target)) || offCanvasDismiss && offCanvasDismiss.contains(target))) {
      self.relatedTarget = offCanvasDismiss && offCanvasDismiss.contains(target) ? offCanvasDismiss : void 0;
      self.hide();
    }
    if (trigger && trigger.tagName === "A") e2.preventDefault();
  };
  const offcanvasKeyDismissHandler = ({ code, target }) => {
    const element = Ho(
      offcanvasActiveSelector,
      d(target)
    );
    const self = element && getOffcanvasInstance(element);
    if (!self) return;
    if (self.options.keyboard && code === fn) {
      self.relatedTarget = void 0;
      self.hide();
    }
  };
  const showOffcanvasComplete = (self) => {
    const { element } = self;
    qn(element, offcanvasTogglingClass);
    Qn(element, $);
    Wn(element, ze, "true");
    Wn(element, "role", "dialog");
    G(element, shownOffcanvasEvent);
    toggleOffCanvasDismiss(self, true);
    ro(element);
    yo(element);
  };
  const hideOffcanvasComplete = (self) => {
    const { element, triggers } = self;
    Wn(element, $, "true");
    Qn(element, ze);
    Qn(element, "role");
    vo(element, { visibility: "" });
    const visibleTrigger = showOffcanvasEvent.relatedTarget || triggers.find(isVisible);
    if (visibleTrigger) ro(visibleTrigger);
    removeOverlay(element);
    G(element, hiddenOffcanvasEvent);
    qn(element, offcanvasTogglingClass);
    yo(element);
    if (!getCurrentOpen(element)) {
      toggleOffCanvasDismiss(self);
    }
  };
  class Offcanvas extends BaseComponent {
    static selector = offcanvasSelector;
    static init = offcanvasInitCallback;
    static getInstance = getOffcanvasInstance;
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.triggers = [
        ...ue(
          offcanvasToggleSelector,
          d(element)
        )
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.relatedTarget = void 0;
      this._toggleEventListeners(true);
    }
    get name() {
      return offcanvasComponent;
    }
    get defaults() {
      return offcanvasDefaults;
    }
    toggle() {
      if (Gn(this.element, showClass)) this.hide();
      else this.show();
    }
    show() {
      const { element, options, relatedTarget } = this;
      let overlayDelay = 0;
      if (Gn(element, showClass)) return;
      showOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      shownOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      G(element, showOffcanvasEvent);
      if (showOffcanvasEvent.defaultPrevented) return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getOffcanvasInstance(currentOpen) || Xn(
          currentOpen,
          modalComponent
        );
        if (that) that.hide();
      }
      if (options.backdrop) {
        if (!hasPopup(overlay)) appendOverlay(element, true);
        else toggleOverlayType();
        overlayDelay = ae(overlay);
        showOverlay();
        setTimeout(() => beforeOffcanvasShow(this), overlayDelay);
      } else {
        beforeOffcanvasShow(this);
        if (currentOpen && Gn(overlay, showClass)) hideOverlay();
      }
    }
    hide() {
      const { element, relatedTarget } = this;
      if (!Gn(element, showClass)) return;
      hideOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      hiddenOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      G(element, hideOffcanvasEvent);
      if (hideOffcanvasEvent.defaultPrevented) return;
      Kn(element, offcanvasTogglingClass);
      qn(element, showClass);
      beforeOffcanvasHide(this);
    }
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      this.triggers.forEach((btn) => {
        action(btn, gt, offcanvasTriggerHandler);
      });
    };
    dispose() {
      const { element } = this;
      const isOpen = Gn(element, showClass);
      const callback = () => setTimeout(() => super.dispose(), 1);
      this.hide();
      this._toggleEventListeners();
      if (isOpen) no(element, callback);
      else callback();
    }
  }
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
    requestAnimationFrame(() => {
      const tipClasses = /\b(top|bottom|start|end)+/;
      const { element, tooltip, container, offsetParent, options, arrow } = self;
      if (!tooltip) return;
      const RTL = Bo(element);
      const { x: scrollLeft, y: scrollTop } = So(offsetParent);
      vo(tooltip, {
        top: "",
        left: "",
        right: "",
        bottom: ""
      });
      const { offsetWidth: tipWidth, offsetHeight: tipHeight } = tooltip;
      const { clientWidth: htmlcw, clientHeight: htmlch, offsetWidth: htmlow } = w$1(element);
      let { placement } = options;
      const { clientWidth: parentCWidth, offsetWidth: parentOWidth } = container;
      const parentPosition = f$1(
        container,
        "position"
      );
      const fixedParent = parentPosition === "fixed";
      const scrollbarWidth = fixedParent ? Math.abs(parentCWidth - parentOWidth) : Math.abs(htmlcw - htmlow);
      const leftBoundry = RTL && fixedParent ? scrollbarWidth : 0;
      const rightBoundry = htmlcw - (!RTL ? scrollbarWidth : 0) - 1;
      const observerEntry = self._observer.getEntry(element);
      const {
        width: elemWidth,
        height: elemHeight,
        left: elemRectLeft,
        right: elemRectRight,
        top: elemRectTop
      } = observerEntry?.boundingClientRect || y(element, true);
      const {
        x: elemOffsetLeft,
        y: elemOffsetTop
      } = No(
        element,
        offsetParent,
        { x: scrollLeft, y: scrollTop }
      );
      vo(arrow, {
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
        tooltip.className = tooltip.className.replace(
          tipClasses,
          tipClassPositions[placement]
        );
      }
      if (horizontals.includes(placement)) {
        if (placement === "left") {
          leftPosition = elemOffsetLeft - tipWidth - arrowWidth;
        } else {
          leftPosition = elemOffsetLeft + elemWidth + arrowWidth;
        }
        if (topExceed && bottomExceed) {
          topPosition = 0;
          bottomPosition = 0;
          arrowTop = elemOffsetTop + elemHeight / 2 - arrowHeight / 2;
        } else if (topExceed) {
          topPosition = elemOffsetTop;
          bottomPosition = "";
          arrowTop = elemHeight / 2 - arrowWidth;
        } else if (bottomExceed) {
          topPosition = elemOffsetTop - tipHeight + elemHeight;
          bottomPosition = "";
          arrowTop = tipHeight - elemHeight / 2 - arrowWidth;
        } else {
          topPosition = elemOffsetTop - tipHeight / 2 + elemHeight / 2;
          arrowTop = tipHeight / 2 - arrowHeight / 2;
        }
      } else if (verticals.includes(placement)) {
        if (placement === "top") {
          topPosition = elemOffsetTop - tipHeight - arrowHeight;
        } else {
          topPosition = elemOffsetTop + elemHeight + arrowHeight;
        }
        if (leftExceed) {
          leftPosition = 0;
          arrowLeft = elemOffsetLeft + elemWidth / 2 - arrowAdjust;
        } else if (rightExceed) {
          leftPosition = "auto";
          rightPosition = 0;
          arrowRight = elemWidth / 2 + rightBoundry - elemRectRight - arrowAdjust;
        } else {
          leftPosition = elemOffsetLeft - tipWidth / 2 + elemWidth / 2;
          arrowLeft = tipWidth / 2 - arrowAdjust;
        }
      }
      vo(tooltip, {
        top: `${topPosition}px`,
        bottom: bottomPosition === "" ? "" : `${bottomPosition}px`,
        left: leftPosition === "auto" ? leftPosition : `${leftPosition}px`,
        right: rightPosition !== "" ? `${rightPosition}px` : ""
      });
      if (b(arrow)) {
        if (arrowTop !== "") {
          arrow.style.top = `${arrowTop}px`;
        }
        if (arrowLeft !== "") {
          arrow.style.left = `${arrowLeft}px`;
        } else if (arrowRight !== "") {
          arrow.style.right = `${arrowRight}px`;
        }
      }
      const updatedTooltipEvent = po(
        `updated.bs.${Eo(self.name)}`
      );
      G(element, updatedTooltipEvent);
    });
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
    if (N(content) && content.length) {
      let dirty = content.trim();
      if (Lo(sanitizeFn)) dirty = sanitizeFn(dirty);
      const domParser = new DOMParser();
      const tempDocument = domParser.parseFromString(dirty, "text/html");
      element.append(...[...tempDocument.body.childNodes]);
    } else if (b(content)) {
      element.append(content);
    } else if (Fo(content) || we(content) && content.every(u)) {
      element.append(...[...content]);
    }
  };
  const createTip = (self) => {
    const isTooltip = self.name === tooltipComponent;
    const { id, element, options } = self;
    const {
      title,
      placement,
      template,
      animation,
      customClass,
      sanitizeFn,
      dismissible,
      content,
      btnClose
    } = options;
    const tipString = isTooltip ? tooltipString : popoverString;
    const tipPositions = { ...tipClassPositions };
    let titleParts = [];
    let contentParts = [];
    if (Bo(element)) {
      tipPositions.left = "end";
      tipPositions.right = "start";
    }
    const placementClass = `bs-${tipString}-${tipPositions[placement]}`;
    let tooltipTemplate;
    if (b(template)) {
      tooltipTemplate = template;
    } else {
      const htmlMarkup = ne("div");
      setHtml(htmlMarkup, template, sanitizeFn);
      tooltipTemplate = htmlMarkup.firstChild;
    }
    if (!b(tooltipTemplate)) return;
    self.tooltip = tooltipTemplate.cloneNode(true);
    const { tooltip } = self;
    Wn(tooltip, "id", id);
    Wn(tooltip, "role", tooltipString);
    const bodyClass = isTooltip ? `${tooltipString}-inner` : `${popoverString}-body`;
    const tooltipHeader = isTooltip ? null : Ho(`.${popoverString}-header`, tooltip);
    const tooltipBody = Ho(`.${bodyClass}`, tooltip);
    self.arrow = Ho(
      `.${tipString}-arrow`,
      tooltip
    );
    const { arrow } = self;
    if (b(title)) titleParts = [title.cloneNode(true)];
    else {
      const tempTitle = ne("div");
      setHtml(tempTitle, title, sanitizeFn);
      titleParts = [...[...tempTitle.childNodes]];
    }
    if (b(content)) contentParts = [content.cloneNode(true)];
    else {
      const tempContent = ne("div");
      setHtml(tempContent, content, sanitizeFn);
      contentParts = [...[...tempContent.childNodes]];
    }
    if (dismissible) {
      if (title) {
        if (b(btnClose)) {
          titleParts = [...titleParts, btnClose.cloneNode(true)];
        } else {
          const tempBtn = ne("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          titleParts = [...titleParts, tempBtn.firstChild];
        }
      } else {
        if (tooltipHeader) tooltipHeader.remove();
        if (b(btnClose)) {
          contentParts = [...contentParts, btnClose.cloneNode(true)];
        } else {
          const tempBtn = ne("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          contentParts = [...contentParts, tempBtn.firstChild];
        }
      }
    }
    if (!isTooltip) {
      if (title && tooltipHeader) {
        setHtml(tooltipHeader, titleParts, sanitizeFn);
      }
      if (content && tooltipBody) {
        setHtml(tooltipBody, contentParts, sanitizeFn);
      }
      self.btn = Ho(".btn-close", tooltip) || void 0;
    } else if (title && tooltipBody) setHtml(tooltipBody, title, sanitizeFn);
    Kn(tooltip, "position-absolute");
    Kn(arrow, "position-absolute");
    if (!Gn(tooltip, tipString)) Kn(tooltip, tipString);
    if (animation && !Gn(tooltip, fadeClass)) {
      Kn(tooltip, fadeClass);
    }
    if (customClass && !Gn(tooltip, customClass)) {
      Kn(tooltip, customClass);
    }
    if (!Gn(tooltip, placementClass)) Kn(tooltip, placementClass);
  };
  const getElementContainer = (element) => {
    const majorBlockTags = ["HTML", "BODY"];
    const containers = [];
    let { parentNode } = element;
    while (parentNode && !majorBlockTags.includes(parentNode.nodeName)) {
      parentNode = k$1(parentNode);
      if (!(pe(parentNode) || me(parentNode))) {
        containers.push(parentNode);
      }
    }
    return containers.find((c, i2) => {
      if ((f$1(c, "position") !== "relative" || f$1(c, "position") === "relative" && c.offsetHeight !== c.scrollHeight) && containers.slice(i2 + 1).every(
        (r2) => f$1(r2, "position") === "static"
      )) {
        return c;
      }
      return null;
    }) || d(element).body;
  };
  const tooltipSelector = `[${dataBsToggle}="${tooltipString}"],[data-tip="${tooltipString}"]`;
  const titleAttr = "title";
  let getTooltipInstance = (element) => Xn(element, tooltipComponent);
  const tooltipInitCallback = (element) => new Tooltip(element);
  const removeTooltip = (self) => {
    const { element, tooltip, container } = self;
    Qn(element, Me);
    removePopup(
      tooltip,
      container
    );
  };
  const hasTip = (self) => {
    const { tooltip, container } = self;
    return tooltip && hasPopup(tooltip, container);
  };
  const disposeTooltipComplete = (self, callback) => {
    const { element } = self;
    self._toggleEventListeners();
    if (ee(element, dataOriginalTitle) && self.name === tooltipComponent) {
      toggleTooltipTitle(self);
    }
    if (callback) callback();
  };
  const toggleTooltipAction = (self, add) => {
    const action = add ? E : r;
    const { element } = self;
    action(
      d(element),
      Wt,
      self.handleTouch,
      go
    );
  };
  const tooltipShownAction = (self) => {
    const { element } = self;
    const shownTooltipEvent = po(
      `shown.bs.${Eo(self.name)}`
    );
    toggleTooltipAction(self, true);
    G(element, shownTooltipEvent);
    bo.clear(element, "in");
  };
  const tooltipHiddenAction = (self) => {
    const { element } = self;
    const hiddenTooltipEvent = po(
      `hidden.bs.${Eo(self.name)}`
    );
    toggleTooltipAction(self);
    removeTooltip(self);
    G(element, hiddenTooltipEvent);
    bo.clear(element, "out");
  };
  const toggleTooltipOpenHandlers = (self, add) => {
    const action = add ? E : r;
    const { element, tooltip } = self;
    const parentModal = Se(element, `.${modalString}`);
    const parentOffcanvas = Se(element, `.${offcanvasString}`);
    if (add) {
      [element, tooltip].forEach((target) => self._observer.observe(target));
    } else self._observer.disconnect();
    if (parentModal) {
      action(parentModal, `hide.bs.${modalString}`, self.handleHide);
    }
    if (parentOffcanvas) {
      action(parentOffcanvas, `hide.bs.${offcanvasString}`, self.handleHide);
    }
  };
  const toggleTooltipTitle = (self, content) => {
    const titleAtt = [dataOriginalTitle, titleAttr];
    const { element } = self;
    Wn(
      element,
      titleAtt[content ? 0 : 1],
      content || j(element, titleAtt[0]) || ""
    );
    Qn(element, titleAtt[content ? 1 : 0]);
  };
  class Tooltip extends BaseComponent {
    static selector = tooltipSelector;
    static init = tooltipInitCallback;
    static getInstance = getTooltipInstance;
    static styleTip = styleTip;
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      const isTooltip = this.name === tooltipComponent;
      const tipString = isTooltip ? tooltipString : popoverString;
      const tipComponent = isTooltip ? tooltipComponent : popoverComponent;
      getTooltipInstance = (elem) => Xn(elem, tipComponent);
      this.enabled = true;
      this.id = `${tipString}-${ye(element, tipString)}`;
      const { options } = this;
      if (!options.title && isTooltip || !isTooltip && !options.content) {
        return;
      }
      C(tooltipDefaults, { titleAttr: "" });
      if (ee(element, titleAttr) && isTooltip && typeof options.title === "string") {
        toggleTooltipTitle(this, options.title);
      }
      const container = getElementContainer(element);
      const offsetParent = ["sticky", "fixed", "relative"].some(
        (position) => f$1(container, "position") === position
      ) ? container : ge(element);
      this.container = container;
      this.offsetParent = offsetParent;
      createTip(this);
      if (!this.tooltip) return;
      this._observer = new v(() => this.update());
      this._toggleEventListeners(true);
    }
    get name() {
      return tooltipComponent;
    }
    get defaults() {
      return tooltipDefaults;
    }
    handleFocus = () => ro(this.element);
    handleShow = () => this.show();
    show() {
      const { options, tooltip, element, container, id } = this;
      const { animation } = options;
      const outTimer = bo.get(element, "out");
      bo.clear(element, "out");
      if (tooltip && !outTimer && !hasTip(this)) {
        bo.set(
          element,
          () => {
            const showTooltipEvent = po(
              `show.bs.${Eo(this.name)}`
            );
            G(element, showTooltipEvent);
            if (!showTooltipEvent.defaultPrevented) {
              appendPopup(tooltip, container);
              Wn(element, Me, `#${id}`);
              this.update();
              toggleTooltipOpenHandlers(this, true);
              if (!Gn(tooltip, showClass)) Kn(tooltip, showClass);
              if (animation) {
                no(tooltip, () => tooltipShownAction(this));
              } else tooltipShownAction(this);
            }
          },
          17,
          "in"
        );
      }
    }
    handleHide = () => this.hide();
    hide() {
      const { options, tooltip, element } = this;
      const { animation, delay } = options;
      bo.clear(element, "in");
      if (tooltip && hasTip(this)) {
        bo.set(
          element,
          () => {
            const hideTooltipEvent = po(
              `hide.bs.${Eo(this.name)}`
            );
            G(element, hideTooltipEvent);
            if (!hideTooltipEvent.defaultPrevented) {
              this.update();
              qn(tooltip, showClass);
              toggleTooltipOpenHandlers(this);
              if (animation) {
                no(tooltip, () => tooltipHiddenAction(this));
              } else tooltipHiddenAction(this);
            }
          },
          delay + 17,
          "out"
        );
      }
    }
    update = () => {
      styleTip(this);
    };
    toggle = () => {
      const { tooltip } = this;
      if (tooltip && !hasTip(this)) this.show();
      else this.hide();
    };
    enable() {
      const { enabled } = this;
      if (!enabled) {
        this._toggleEventListeners(true);
        this.enabled = !enabled;
      }
    }
    disable() {
      const { tooltip, enabled } = this;
      if (enabled) {
        if (tooltip && hasTip(this)) this.hide();
        this._toggleEventListeners();
        this.enabled = !enabled;
      }
    }
    toggleEnabled() {
      if (!this.enabled) this.enable();
      else this.disable();
    }
    handleTouch = ({ target }) => {
      const { tooltip, element } = this;
      if (tooltip && tooltip.contains(target) || target === element || target && element.contains(target)) ;
      else {
        this.hide();
      }
    };
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      const { element, options, btn } = this;
      const { trigger } = options;
      const isPopover = this.name !== tooltipComponent;
      const dismissible = isPopover && options.dismissible ? true : false;
      if (!trigger.includes("manual")) {
        this.enabled = !!add;
        const triggerOptions = trigger.split(" ");
        triggerOptions.forEach((tr) => {
          if (tr === Et) {
            action(element, vt, this.handleShow);
            action(element, ht, this.handleShow);
            if (!dismissible) {
              action(element, yt, this.handleHide);
              action(
                d(element),
                Wt,
                this.handleTouch,
                go
              );
            }
          } else if (tr === gt) {
            action(element, tr, !dismissible ? this.toggle : this.handleShow);
          } else if (tr === st) {
            action(element, rt, this.handleShow);
            if (!dismissible) action(element, ct, this.handleHide);
            if (On()) {
              action(element, gt, this.handleFocus);
            }
          }
          if (dismissible && btn) {
            action(btn, gt, this.handleHide);
          }
        });
      }
    };
    dispose() {
      const { tooltip, options } = this;
      const clone = { ...this, name: this.name };
      const callback = () => setTimeout(
        () => disposeTooltipComplete(clone, () => super.dispose()),
        17
      );
      if (options.animation && hasTip(clone)) {
        this.options.delay = 0;
        this.hide();
        no(tooltip, callback);
      } else {
        callback();
      }
    }
  }
  const popoverSelector = `[${dataBsToggle}="${popoverString}"],[data-tip="${popoverString}"]`;
  const popoverDefaults = C({}, tooltipDefaults, {
    template: getTipTemplate(popoverString),
    content: "",
    dismissible: false,
    btnClose: '<button class="btn-close position-absolute top-0 end-0 m-1" aria-label="Close"></button>'
  });
  const getPopoverInstance = (element) => Xn(element, popoverComponent);
  const popoverInitCallback = (element) => new Popover(element);
  class Popover extends Tooltip {
    static selector = popoverSelector;
    static init = popoverInitCallback;
    static getInstance = getPopoverInstance;
    static styleTip = styleTip;
    constructor(target, config) {
      super(target, config);
    }
    get name() {
      return popoverComponent;
    }
    get defaults() {
      return popoverDefaults;
    }
    show = () => {
      super.show();
      const { options, btn } = this;
      if (options.dismissible && btn) setTimeout(() => ro(btn), 17);
    };
  }
  const tabString = "tab";
  const tabComponent = "Tab";
  const tabSelector = `[${dataBsToggle}="${tabString}"]`;
  const getTabInstance = (element) => Xn(element, tabComponent);
  const tabInitCallback = (element) => new Tab(element);
  const showTabEvent = po(
    `show.bs.${tabString}`
  );
  const shownTabEvent = po(
    `shown.bs.${tabString}`
  );
  const hideTabEvent = po(
    `hide.bs.${tabString}`
  );
  const hiddenTabEvent = po(
    `hidden.bs.${tabString}`
  );
  const tabPrivate = /* @__PURE__ */ new Map();
  const triggerTabEnd = (self) => {
    const { tabContent, nav } = self;
    if (tabContent && Gn(tabContent, collapsingClass)) {
      tabContent.style.height = "";
      qn(tabContent, collapsingClass);
    }
    if (nav) bo.clear(nav);
  };
  const triggerTabShow = (self) => {
    const { element, tabContent, content: nextContent, nav } = self;
    const { tab } = b(nav) && tabPrivate.get(nav) || { tab: null };
    if (tabContent && nextContent && Gn(nextContent, fadeClass)) {
      const { currentHeight, nextHeight } = tabPrivate.get(element) || { currentHeight: 0, nextHeight: 0 };
      if (currentHeight !== nextHeight) {
        setTimeout(() => {
          tabContent.style.height = `${nextHeight}px`;
          mo(tabContent);
          no(tabContent, () => triggerTabEnd(self));
        }, 50);
      } else {
        triggerTabEnd(self);
      }
    } else if (nav) bo.clear(nav);
    shownTabEvent.relatedTarget = tab;
    G(element, shownTabEvent);
  };
  const triggerTabHide = (self) => {
    const { element, content: nextContent, tabContent, nav } = self;
    const { tab, content } = nav && tabPrivate.get(nav) || { tab: null, content: null };
    let currentHeight = 0;
    if (tabContent && nextContent && Gn(nextContent, fadeClass)) {
      [content, nextContent].forEach((c) => {
        if (c) Kn(c, "overflow-hidden");
      });
      currentHeight = content ? content.scrollHeight : 0;
    }
    showTabEvent.relatedTarget = tab;
    hiddenTabEvent.relatedTarget = element;
    G(element, showTabEvent);
    if (showTabEvent.defaultPrevented) return;
    if (nextContent) Kn(nextContent, activeClass);
    if (content) qn(content, activeClass);
    if (tabContent && nextContent && Gn(nextContent, fadeClass)) {
      const nextHeight = nextContent.scrollHeight;
      tabPrivate.set(element, {
        currentHeight,
        nextHeight,
        tab: null,
        content: null
      });
      Kn(tabContent, collapsingClass);
      tabContent.style.height = `${currentHeight}px`;
      mo(tabContent);
      [content, nextContent].forEach((c) => {
        if (c) qn(c, "overflow-hidden");
      });
    }
    if (nextContent && nextContent && Gn(nextContent, fadeClass)) {
      setTimeout(() => {
        Kn(nextContent, showClass);
        no(nextContent, () => {
          triggerTabShow(self);
        });
      }, 1);
    } else {
      if (nextContent) Kn(nextContent, showClass);
      triggerTabShow(self);
    }
    if (tab) G(tab, hiddenTabEvent);
  };
  const getActiveTab = (self) => {
    const { nav } = self;
    if (!b(nav)) {
      return { tab: null, content: null };
    }
    const activeTabs = Ro(
      activeClass,
      nav
    );
    let tab = null;
    if (activeTabs.length === 1 && !dropdownMenuClasses.some(
      (c) => Gn(activeTabs[0].parentElement, c)
    )) {
      [tab] = activeTabs;
    } else if (activeTabs.length > 1) {
      tab = activeTabs[activeTabs.length - 1];
    }
    const content = b(tab) ? getTargetElement(tab) : null;
    return { tab, content };
  };
  const getParentDropdown = (element) => {
    if (!b(element)) return null;
    const dropdown = Se(element, `.${dropdownMenuClasses.join(",.")}`);
    return dropdown ? Ho(`.${dropdownMenuClasses[0]}-toggle`, dropdown) : null;
  };
  const tabClickHandler = (e2) => {
    const element = Se(e2.target, tabSelector);
    const self = element && getTabInstance(element);
    if (!self) return;
    e2.preventDefault();
    self.show();
  };
  class Tab extends BaseComponent {
    static selector = tabSelector;
    static init = tabInitCallback;
    static getInstance = getTabInstance;
    constructor(target) {
      super(target);
      const { element } = this;
      const content = getTargetElement(element);
      if (!content) return;
      const nav = Se(element, ".nav");
      const container = Se(
        content,
        ".tab-content"
      );
      this.nav = nav;
      this.content = content;
      this.tabContent = container;
      this.dropdown = getParentDropdown(element);
      const { tab } = getActiveTab(this);
      if (nav && !tab) {
        const firstTab = Ho(tabSelector, nav);
        const firstTabContent = firstTab && getTargetElement(firstTab);
        if (firstTabContent) {
          Kn(firstTab, activeClass);
          Kn(firstTabContent, showClass);
          Kn(firstTabContent, activeClass);
          Wn(element, Pe, "true");
        }
      }
      this._toggleEventListeners(true);
    }
    get name() {
      return tabComponent;
    }
    show() {
      const { element, content: nextContent, nav, dropdown } = this;
      if (nav && bo.get(nav) || Gn(element, activeClass)) return;
      const { tab, content } = getActiveTab(this);
      if (nav && tab) {
        tabPrivate.set(nav, { tab, content, currentHeight: 0, nextHeight: 0 });
      }
      hideTabEvent.relatedTarget = element;
      if (!b(tab)) return;
      G(tab, hideTabEvent);
      if (hideTabEvent.defaultPrevented) return;
      Kn(element, activeClass);
      Wn(element, Pe, "true");
      const activeDropdown = b(tab) && getParentDropdown(tab);
      if (activeDropdown && Gn(activeDropdown, activeClass)) {
        qn(activeDropdown, activeClass);
      }
      if (nav) {
        const toggleTab = () => {
          if (tab) {
            qn(tab, activeClass);
            Wn(tab, Pe, "false");
          }
          if (dropdown && !Gn(dropdown, activeClass)) {
            Kn(dropdown, activeClass);
          }
        };
        if (content && (Gn(content, fadeClass) || nextContent && Gn(nextContent, fadeClass))) {
          bo.set(nav, toggleTab, 1);
        } else toggleTab();
      }
      if (content) {
        qn(content, showClass);
        if (Gn(content, fadeClass)) {
          no(content, () => triggerTabHide(this));
        } else {
          triggerTabHide(this);
        }
      }
    }
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      action(this.element, gt, tabClickHandler);
    };
    dispose() {
      this._toggleEventListeners();
      super.dispose();
    }
  }
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
  const getToastInstance = (element) => Xn(element, toastComponent);
  const toastInitCallback = (element) => new Toast(element);
  const showToastEvent = po(
    `show.bs.${toastString}`
  );
  const shownToastEvent = po(
    `shown.bs.${toastString}`
  );
  const hideToastEvent = po(
    `hide.bs.${toastString}`
  );
  const hiddenToastEvent = po(
    `hidden.bs.${toastString}`
  );
  const showToastComplete = (self) => {
    const { element, options } = self;
    qn(element, showingClass);
    bo.clear(element, showingClass);
    G(element, shownToastEvent);
    if (options.autohide) {
      bo.set(element, () => self.hide(), options.delay, toastString);
    }
  };
  const hideToastComplete = (self) => {
    const { element } = self;
    qn(element, showingClass);
    qn(element, showClass);
    Kn(element, hideClass);
    bo.clear(element, toastString);
    G(element, hiddenToastEvent);
  };
  const hideToast = (self) => {
    const { element, options } = self;
    Kn(element, showingClass);
    if (options.animation) {
      mo(element);
      no(element, () => hideToastComplete(self));
    } else {
      hideToastComplete(self);
    }
  };
  const showToast = (self) => {
    const { element, options } = self;
    bo.set(
      element,
      () => {
        qn(element, hideClass);
        mo(element);
        Kn(element, showClass);
        Kn(element, showingClass);
        if (options.animation) {
          no(element, () => showToastComplete(self));
        } else {
          showToastComplete(self);
        }
      },
      17,
      showingClass
    );
  };
  function toastClickHandler(e2) {
    const element = getTargetElement(this);
    const self = element && getToastInstance(element);
    if (isDisabled(this)) return;
    if (!self) return;
    if (this.tagName === "A") e2.preventDefault();
    self.relatedTarget = this;
    self.show();
  }
  const interactiveToastHandler = (e2) => {
    const element = e2.target;
    const self = getToastInstance(element);
    const { type, relatedTarget } = e2;
    if (!self || element === relatedTarget || element.contains(relatedTarget)) return;
    if ([ht, rt].includes(type)) {
      bo.clear(element, toastString);
    } else {
      bo.set(element, () => self.hide(), self.options.delay, toastString);
    }
  };
  class Toast extends BaseComponent {
    static selector = toastSelector;
    static init = toastInitCallback;
    static getInstance = getToastInstance;
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      if (options.animation && !Gn(element, fadeClass)) {
        Kn(element, fadeClass);
      } else if (!options.animation && Gn(element, fadeClass)) {
        qn(element, fadeClass);
      }
      this.dismiss = Ho(toastDismissSelector, element);
      this.triggers = [
        ...ue(
          toastToggleSelector,
          d(element)
        )
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this._toggleEventListeners(true);
    }
    get name() {
      return toastComponent;
    }
    get defaults() {
      return toastDefaults;
    }
    get isShown() {
      return Gn(this.element, showClass);
    }
    show = () => {
      const { element, isShown } = this;
      if (!element || isShown) return;
      G(element, showToastEvent);
      if (!showToastEvent.defaultPrevented) showToast(this);
    };
    hide = () => {
      const { element, isShown } = this;
      if (!element || !isShown) return;
      G(element, hideToastEvent);
      if (!hideToastEvent.defaultPrevented) hideToast(this);
    };
    _toggleEventListeners = (add) => {
      const action = add ? E : r;
      const { element, triggers, dismiss, options, hide } = this;
      if (dismiss) {
        action(dismiss, gt, hide);
      }
      if (options.autohide) {
        [rt, ct, ht, yt].forEach(
          (e2) => action(element, e2, interactiveToastHandler)
        );
      }
      if (triggers.length) {
        triggers.forEach((btn) => {
          action(btn, gt, toastClickHandler);
        });
      }
    };
    dispose() {
      const { element, isShown } = this;
      this._toggleEventListeners();
      bo.clear(element, toastString);
      if (isShown) qn(element, showClass);
      super.dispose();
    }
  }
  const componentsList = /* @__PURE__ */ new Map();
  [
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
  ].forEach((c) => componentsList.set(c.prototype.name, c));
  const initComponentDataAPI = (callback, collection) => {
    [...collection].forEach((x2) => callback(x2));
  };
  const removeComponentDataAPI = (component, context) => {
    const compData = L.getAllFor(component);
    if (compData) {
      [...compData].forEach(([element, instance]) => {
        if (context.contains(element)) {
          instance.dispose();
        }
      });
    }
  };
  const initCallback = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    const elemCollection = [...ke("*", lookUp)];
    componentsList.forEach((cs) => {
      const { init, selector } = cs;
      initComponentDataAPI(
        init,
        elemCollection.filter((item) => ve(item, selector))
      );
    });
  };
  const removeDataAPI = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    componentsList.forEach((comp) => {
      removeComponentDataAPI(comp.prototype.name, lookUp);
    });
  };
  if (document.body) initCallback();
  else {
    E(document, "DOMContentLoaded", () => initCallback(), {
      once: true
    });
  }
  exports.Alert = Alert;
  exports.Button = Button;
  exports.Carousel = Carousel;
  exports.Collapse = Collapse;
  exports.Dropdown = Dropdown;
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
