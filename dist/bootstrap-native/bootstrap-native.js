var BSN = function(exports) {
  "use strict";var __defProp = Object.defineProperty;
var __defNormalProp = (obj, key, value) => key in obj ? __defProp(obj, key, { enumerable: true, configurable: true, writable: true, value }) : obj[key] = value;
var __publicField = (obj, key, value) => __defNormalProp(obj, typeof key !== "symbol" ? key + "" : key, value);

  const e = {}, f = (t) => {
    const { type: c, currentTarget: i2 } = t;
    [...e[c]].forEach(([n, s]) => {
      /* istanbul ignore else @preserve */
      i2 === n && [...s].forEach(([o, a]) => {
        o.apply(n, [t]), typeof a == "object" && a.once && r(n, c, o, a);
      });
    });
  }, E$1 = (t, c, i2, n) => {
    /* istanbul ignore else @preserve */
    e[c] || (e[c] = /* @__PURE__ */ new Map());
    const s = e[c];
    /* istanbul ignore else @preserve */
    s.has(t) || s.set(t, /* @__PURE__ */ new Map());
    const o = s.get(t), { size: a } = o;
    o.set(i2, n);
    /* istanbul ignore else @preserve */
    a || t.addEventListener(c, f, n);
  }, r = (t, c, i2, n) => {
    const s = e[c], o = s && s.get(t), a = o && o.get(i2), d2 = a !== void 0 ? a : n;
    /* istanbul ignore else @preserve */
    o && o.has(i2) && o.delete(i2);
    /* istanbul ignore else @preserve */
    s && (!o || !o.size) && s.delete(t);
    /* istanbul ignore else @preserve */
    (!s || !s.size) && delete e[c];
    /* istanbul ignore else @preserve */
    (!o || !o.size) && t.removeEventListener(
      c,
      f,
      d2
    );
  }, g$1 = E$1, M$1 = r;
  const eventListener = /* @__PURE__ */ Object.freeze(/* @__PURE__ */ Object.defineProperty({
    __proto__: null,
    addListener: E$1,
    globalListener: f,
    off: M$1,
    on: g$1,
    registry: e,
    removeListener: r
  }, Symbol.toStringTag, { value: "Module" }));
  const we = "aria-describedby", Ae = "aria-expanded", X = "aria-hidden", Te = "aria-modal", ke = "aria-pressed", De = "aria-selected", P = "DOMContentLoaded", ot = "focus", st = "focusin", ct = "focusout", ut = "keydown", dt = "keyup", ft = "click", gt = "mousedown", Et = "hover", bt = "mouseenter", ht = "mouseleave", Dt = "pointerdown", Ot = "pointermove", Lt = "pointerup", zt = "resize", Ht = "scroll", Ut = "touchstart", Ve = "dragstart", qt = 'a[href], button, input, textarea, select, details, [tabindex]:not([tabindex="-1"]', Je = "ArrowDown", Xe = "ArrowUp", Ye = "ArrowLeft", Ze = "ArrowRight", sn = "Escape", Jt = "transitionDuration", Xt = "transitionDelay", C = "transitionend", W = "transitionProperty", Yt = navigator.userAgentData, A = Yt, { userAgent: Zt } = navigator, S = Zt, z = /iPhone|iPad|iPod|Android/i;
  // istanbul ignore else @preserve
  A ? A.brands.some((t) => z.test(t.brand)) : z.test(S);
  const V = /(iPhone|iPod|iPad)/, An = A ? A.brands.some((t) => V.test(t.brand)) : (
    /* istanbul ignore next @preserve */
    V.test(S)
  );
  S ? S.includes("Firefox") : (
    /* istanbul ignore next @preserve */
    false
  );
  const { head: N } = document;
  ["webkitPerspective", "perspective"].some((t) => t in N.style);
  const R = (t, e2, n, o) => {
    const s = o || false;
    t.addEventListener(e2, n, s);
  }, Q = (t, e2, n, o) => {
    const s = o || false;
    t.removeEventListener(e2, n, s);
  }, $t = (t, e2, n, o) => {
    const s = (c) => {
      /* istanbul ignore else @preserve */
      (c.target === t || c.currentTarget === t) && (n.apply(t, [c]), Q(t, e2, s, o));
    };
    R(t, e2, s, o);
  }, _t = () => {
  };
  (() => {
    let t = false;
    try {
      const e2 = Object.defineProperty({}, "passive", {
        get: () => (t = true, t)
      });
      // istanbul ignore next @preserve
      $t(document, P, _t, e2);
    } catch {
    }
    return t;
  })();
  ["webkitTransform", "transform"].some((t) => t in N.style);
  ["webkitAnimation", "animation"].some((t) => t in N.style);
  ["webkitTransition", "transition"].some((t) => t in N.style);
  const j = (t, e2) => t.getAttribute(e2), te = (t, e2) => t.hasAttribute(e2), In = (t, e2, n) => t.setAttribute(e2, n), zn = (t, e2) => t.removeAttribute(e2), Bn = (t, ...e2) => {
    t.classList.add(...e2);
  }, Fn = (t, ...e2) => {
    t.classList.remove(...e2);
  }, Hn = (t, e2) => t.classList.contains(e2), v = (t) => t != null && typeof t == "object" || false, i = (t) => v(t) && typeof t.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some((e2) => t.nodeType === e2) || false, l = (t) => i(t) && t.nodeType === 1 || false, E = /* @__PURE__ */ new Map(), L = {
    data: E,
    /**
     * Sets web components data.
     *
     * @param element target element
     * @param component the component's name or a unique key
     * @param instance the component instance
     */
    set: (t, e2, n) => {
      if (!l(t)) return;
      // istanbul ignore else @preserve
      E.has(e2) || E.set(e2, /* @__PURE__ */ new Map()), E.get(e2).set(t, n);
    },
    /**
     * Returns all instances for specified component.
     *
     * @param component the component's name or a unique key
     * @returns all the component instances
     */
    getAllFor: (t) => E.get(t) || null,
    /**
     * Returns the instance associated with the target.
     *
     * @param element target element
     * @param component the component's name or a unique key
     * @returns the instance
     */
    get: (t, e2) => {
      if (!l(t) || !e2) return null;
      const n = L.getAllFor(e2);
      return t && n && n.get(t) || null;
    },
    /**
     * Removes web components data.
     *
     * @param element target element
     * @param component the component's name or a unique key
     */
    remove: (t, e2) => {
      const n = L.getAllFor(e2);
      if (!n || !l(t)) return;
      n.delete(t);
      // istanbul ignore else @preserve
      n.size === 0 && E.delete(e2);
    }
  }, Rn = (t, e2) => L.get(t, e2), M = (t) => typeof t == "string" || false, q = (t) => v(t) && t.constructor.name === "Window" || false, G = (t) => i(t) && t.nodeType === 9 || false, d = (t) => q(t) ? t.document : G(t) ? t : i(t) ? t.ownerDocument : window.document, T = (t, ...e2) => Object.assign(t, ...e2), ee = (t) => {
    if (!t) return;
    if (M(t))
      return d().createElement(t);
    const { tagName: e2 } = t, n = ee(e2);
    if (!n) return;
    const o = { ...t };
    return delete o.tagName, T(n, o);
  }, K = (t, e2) => t.dispatchEvent(e2), g = (t, e2) => {
    const n = getComputedStyle(t), o = e2.replace("webkit", "Webkit").replace(/([A-Z])/g, "-$1").toLowerCase();
    return n.getPropertyValue(o);
  }, ce = (t) => {
    const e2 = g(t, W), n = g(t, Xt), o = n.includes("ms") ? (
      /* istanbul ignore next */
      1
    ) : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : (
      /* istanbul ignore next */
      0
    );
    return Number.isNaN(s) ? (
      /* istanbul ignore next */
      0
    ) : s;
  }, re = (t) => {
    const e2 = g(t, W), n = g(t, Jt), o = n.includes("ms") ? (
      /* istanbul ignore next */
      1
    ) : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : (
      /* istanbul ignore next */
      0
    );
    return Number.isNaN(s) ? (
      /* istanbul ignore next */
      0
    ) : s;
  }, qn = (t, e2) => {
    let n = 0;
    const o = new Event(C), s = re(t), c = ce(t);
    if (s) {
      const a = (u) => {
        // istanbul ignore else @preserve
        u.target === t && (e2.apply(t, [u]), t.removeEventListener(C, a), n = 1);
      };
      t.addEventListener(C, a), setTimeout(() => {
        // istanbul ignore next @preserve
        n || K(t, o);
      }, s + c + 17);
    } else
      e2.apply(t, [o]);
  }, Jn = (t, e2) => t.focus(e2), B = (t) => ["true", true].includes(t) ? true : ["false", false].includes(t) ? false : ["null", "", null, void 0].includes(t) ? null : t !== "" && !Number.isNaN(+t) ? +t : t, w = (t) => Object.entries(t), ae = (t) => t.toLowerCase(), Xn = (t, e2, n, o) => {
    const s = { ...n }, c = { ...t.dataset }, a = { ...e2 }, u = {}, p = "title";
    return w(c).forEach(([r2, f2]) => {
      const y = typeof r2 == "string" && r2.includes(o) ? r2.replace(o, "").replace(/[A-Z]/g, (J) => ae(J)) : (
        /* istanbul ignore next @preserve */
        r2
      );
      u[y] = B(f2);
    }), w(s).forEach(([r2, f2]) => {
      s[r2] = B(f2);
    }), w(e2).forEach(([r2, f2]) => {
      // istanbul ignore else @preserve
      r2 in s ? a[r2] = s[r2] : r2 in u ? a[r2] = u[r2] : a[r2] = r2 === p ? j(t, p) : f2;
    }), a;
  }, Zn = (t) => Object.keys(t), $n = (t) => Object.values(t), to = (t, e2) => {
    const n = new CustomEvent(t, {
      cancelable: true,
      bubbles: true
    });
    // istanbul ignore else @preserve
    return v(e2) && T(n, e2), n;
  }, eo = { passive: true }, no = (t) => t.offsetHeight, oo = (t, e2) => {
    w(e2).forEach(([n, o]) => {
      if (o && M(n) && n.includes("--"))
        t.style.setProperty(n, o);
      else {
        const s = {};
        s[n] = o, T(t.style, s);
      }
    });
  }, I = (t) => v(t) && t.constructor.name === "Map" || false, ie = (t) => typeof t == "number" || false, m = /* @__PURE__ */ new Map(), so = {
    /**
     * Sets a new timeout timer for an element, or element -> key association.
     *
     * @param element target element
     * @param callback the callback
     * @param delay the execution delay
     * @param key a unique key
     */
    set: (t, e2, n, o) => {
      if (!l(t)) return;
      // istanbul ignore else @preserve
      if (o && o.length) {
        // istanbul ignore else @preserve
        m.has(t) || m.set(t, /* @__PURE__ */ new Map()), m.get(t).set(o, setTimeout(e2, n));
      } else
        m.set(t, setTimeout(e2, n));
    },
    /**
     * Returns the timer associated with the target.
     *
     * @param element target element
     * @param key a unique
     * @returns the timer
     */
    get: (t, e2) => {
      if (!l(t)) return null;
      const n = m.get(t);
      return e2 && n && I(n) ? n.get(e2) || /* istanbul ignore next */
      null : ie(n) ? n : null;
    },
    /**
     * Clears the element's timer.
     *
     * @param element target element
     * @param key a unique key
     */
    clear: (t, e2) => {
      if (!l(t)) return;
      const n = m.get(t);
      if (e2 && e2.length && I(n)) {
        clearTimeout(n.get(e2)), n.delete(e2);
        // istanbul ignore else @preserve
        n.size === 0 && m.delete(t);
      } else
        clearTimeout(n), m.delete(t);
    }
  }, ue = (t, e2) => (i(e2) ? e2 : d()).querySelectorAll(t), x = /* @__PURE__ */ new Map();
  function le(t) {
    const { shiftKey: e2, code: n } = t, o = d(this), s = [...ue(qt, this)].filter(
      (u) => !te(u, "disabled") && !j(u, X)
    );
    if (!s.length) return;
    const c = s[0], a = s[s.length - 1];
    // istanbul ignore else @preserve
    n === "Tab" && (e2 && o.activeElement === c ? (a.focus(), t.preventDefault()) : !e2 && o.activeElement === a && (c.focus(), t.preventDefault()));
  }
  const de = (t) => x.has(t) === true, ro = (t) => {
    const e2 = de(t);
    (e2 ? Q : R)(t, "keydown", le), e2 ? x.delete(t) : x.set(t, true);
  }, h = (t, e2) => {
    const { width: n, height: o, top: s, right: c, bottom: a, left: u } = t.getBoundingClientRect();
    let p = 1, r2 = 1;
    if (e2 && l(t)) {
      const { offsetWidth: f2, offsetHeight: y } = t;
      p = f2 > 0 ? Math.round(n) / f2 : (
        /* istanbul ignore next */
        1
      ), r2 = y > 0 ? Math.round(o) / y : (
        /* istanbul ignore next */
        1
      );
    }
    return {
      width: n / p,
      height: o / r2,
      top: s / r2,
      right: c / p,
      bottom: a / r2,
      left: u / p,
      x: u / p,
      y: s / r2
    };
  }, ao = (t) => d(t).body, k = (t) => d(t).documentElement, pe = (t) => i(t) && t.constructor.name === "ShadowRoot" || false, lo = (t) => t.nodeName === "HTML" ? t : l(t) && t.assignedSlot || // step into the shadow DOM of the parent of a slotted node
  i(t) && t.parentNode || // DOM Element detected
  pe(t) && t.host || // ShadowRoot detected
  k(t);
  let F = 0, H = 0;
  const b = /* @__PURE__ */ new Map(), me = (t, e2) => {
    let n = e2 ? F : H;
    if (e2) {
      const o = me(t), s = b.get(o) || /* @__PURE__ */ new Map();
      b.has(o) || b.set(o, s), I(s) && !s.has(e2) ? (s.set(e2, n), F += 1) : n = s.get(e2);
    } else {
      const o = t.id || t;
      b.has(o) ? n = b.get(o) : (b.set(o, n), H += 1);
    }
    return n;
  }, fo = (t) => {
    var e2;
    return t ? G(t) ? t.defaultView : i(t) ? (e2 = t == null ? void 0 : t.ownerDocument) == null ? void 0 : e2.defaultView : t : window;
  }, ge = (t) => Array.isArray(t) || false, vo = (t) => {
    if (!i(t)) return false;
    const { top: e2, bottom: n } = h(t), { clientHeight: o } = k(t);
    return e2 <= o && n >= 0;
  }, ho = (t) => typeof t == "function" || false, Mo = (t) => v(t) && t.constructor.name === "NodeList" || false, To = (t) => k(t).dir === "rtl", Do = (t) => i(t) && ["TABLE", "TD", "TH"].includes(t.nodeName) || false, Ee = (t, e2) => t ? t.closest(e2) || // break out of `ShadowRoot`
  Ee(t.getRootNode().host, e2) : null, Co = (t, e2) => l(t) ? t : (i(e2) ? e2 : d()).querySelector(t), be = (t, e2) => (i(e2) ? e2 : d()).getElementsByTagName(t), Io = (t, e2) => (e2 && i(e2) ? e2 : d()).getElementsByClassName(t), xo = (t, e2) => t.matches(e2);
  const fadeClass = "fade";
  const showClass = "show";
  const dataBsDismiss = "data-bs-dismiss";
  const alertString = "alert";
  const alertComponent = "Alert";
  const version = "5.0.15";
  const Version = version;
  class BaseComponent {
    /**
     * @param target `HTMLElement` or selector string
     * @param config component instance options
     */
    constructor(target, config) {
      /** just to have something to extend from */
      // istanbul ignore next @preserve coverage wise this isn't important
      __publicField(this, "_toggleEventListeners", () => {
      });
      let element;
      try {
        if (l(target)) {
          element = target;
        } else if (M(target)) {
          element = Co(target);
          // istanbul ignore else @preserve
          if (!element) throw Error(`"${target}" is not a valid selector.`);
        } else {
          throw Error(`your target is not an instance of HTMLElement.`);
        }
      } catch (e2) {
        throw Error(`${this.name} Error: ${e2.message}`);
      }
      const prevInstance = L.get(element, this.name);
      // istanbul ignore else @preserve
      if (prevInstance) {
        prevInstance._toggleEventListeners();
      }
      this.element = element;
      this.options = this.defaults && Zn(this.defaults).length ? Xn(element, this.defaults, config || {}, "bs") : {};
      L.set(element, this.name, this);
    }
    // istanbul ignore next @preserve
    get version() {
      return Version;
    }
    // istanbul ignore next @preserve
    get name() {
      return "BaseComponent";
    }
    // istanbul ignore next @preserve
    get defaults() {
      return {};
    }
    /** Removes component from target element. */
    dispose() {
      L.remove(this.element, this.name);
      Zn(this).forEach((prop) => {
        delete this[prop];
      });
    }
  }
  const alertSelector = `.${alertString}`;
  const alertDismissSelector = `[${dataBsDismiss}="${alertString}"]`;
  const getAlertInstance = (element) => Rn(element, alertComponent);
  const alertInitCallback = (element) => new Alert(element);
  const closeAlertEvent = to(
    `close.bs.${alertString}`
  );
  const closedAlertEvent = to(
    `closed.bs.${alertString}`
  );
  const alertTransitionEnd = (self) => {
    const { element } = self;
    K(element, closedAlertEvent);
    self._toggleEventListeners();
    self.dispose();
    element.remove();
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
        // istanbul ignore else @preserve
        if (element && Hn(element, showClass)) {
          K(element, closeAlertEvent);
          if (!closeAlertEvent.defaultPrevented) {
            Fn(element, showClass);
            if (Hn(element, fadeClass)) {
              qn(element, () => alertTransitionEnd(this));
            } else alertTransitionEnd(this);
          }
        }
      });
      /**
       * Toggle on / off the `click` event listener.
       *
       * @param add when `true`, event listener is added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        const { dismiss, close } = this;
        // istanbul ignore else @preserve
        if (dismiss) action(dismiss, ft, close);
      });
      this.dismiss = Co(alertDismissSelector, this.element);
      this._toggleEventListeners(true);
    }
    /** Returns component name string. */
    get name() {
      return alertComponent;
    }
    /** Remove the component from target element. */
    dispose() {
      this._toggleEventListeners();
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
  const getButtonInstance = (element) => Rn(element, buttonComponent);
  const buttonInitCallback = (element) => new Button(element);
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
      __publicField(this, "toggle", (e2) => {
        if (e2) e2.preventDefault();
        const { element, isActive } = this;
        if (!Hn(element, "disabled") && !j(element, "disabled")) {
          const action = isActive ? Fn : Bn;
          action(element, activeClass);
          In(element, ke, isActive ? "false" : "true");
          this.isActive = Hn(element, activeClass);
        }
      });
      // BUTTON PRIVATE METHOD
      // =====================
      /**
       * Toggles on/off the `click` event listener.
       *
       * @param add when `true`, event listener is added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        action(this.element, ft, this.toggle);
      });
      const { element } = this;
      this.isActive = Hn(element, activeClass);
      In(element, ke, String(!!this.isActive));
      this._toggleEventListeners(true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return buttonComponent;
    }
    /** Removes the `Button` component from the target element. */
    dispose() {
      this._toggleEventListeners();
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
    const doc = d(element);
    return targetAttr.map((att) => {
      const attValue = j(element, att);
      if (attValue) {
        return att === dataBsParent ? Ee(element, attValue) : Co(attValue, doc);
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
  const getCarouselInstance = (element) => Rn(element, carouselComponent);
  const carouselInitCallback = (element) => new Carousel(element);
  let startX = 0;
  let currentX = 0;
  let endX = 0;
  const carouselSlideEvent = to(`slide.bs.${carouselString}`);
  const carouselSlidEvent = to(`slid.bs.${carouselString}`);
  const carouselTransitionEndHandler = (self) => {
    const { index, direction, element, slides, options } = self;
    // istanbul ignore else @preserve
    if (self.isAnimating) {
      const activeItem = getActiveIndex(self);
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      Bn(slides[index], activeClass);
      Fn(slides[index], `${carouselItem}-${orientation}`);
      Fn(slides[index], `${carouselItem}-${directionClass}`);
      Fn(slides[activeItem], activeClass);
      Fn(slides[activeItem], `${carouselItem}-${directionClass}`);
      K(element, carouselSlidEvent);
      so.clear(element, dataBsSlide);
      if (self.cycle && !d(element).hidden && options.interval && !self.isPaused) {
        self.cycle();
      }
    }
  };
  function carouselPauseHandler() {
    const self = getCarouselInstance(this);
    // istanbul ignore else @preserve
    if (self && !self.isPaused && !so.get(this, pausedClass)) {
      Bn(this, pausedClass);
    }
  }
  function carouselResumeHandler() {
    const self = getCarouselInstance(this);
    // istanbul ignore else @preserve
    if (self && self.isPaused && !so.get(this, pausedClass)) {
      self.cycle();
    }
  }
  function carouselIndicatorHandler(e2) {
    e2.preventDefault();
    const element = Ee(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    // istanbul ignore else @preserve
    if (self && !self.isAnimating) {
      const newIndex = +(j(this, dataBsSlideTo) || // istanbul ignore next @preserve
      0);
      // istanbul ignore else @preserve
      if (this && !Hn(this, activeClass) && // event target is not active
      !Number.isNaN(newIndex)) {
        self.to(newIndex);
      }
    }
  }
  function carouselControlsHandler(e2) {
    e2.preventDefault();
    const element = Ee(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    // istanbul ignore else @preserve
    if (self && !self.isAnimating) {
      const orientation = j(this, dataBsSlide);
      // istanbul ignore else @preserve
      if (orientation === "next") {
        self.next();
      } else if (orientation === "prev") {
        self.prev();
      }
    }
  }
  const carouselKeyHandler = ({ code, target }) => {
    const doc = d(target);
    const [element] = [...ue(carouselSelector, doc)].filter(
      (x2) => vo(x2)
    );
    const self = getCarouselInstance(element);
    // istanbul ignore next @preserve
    if (self && !self.isAnimating && !/textarea|input/i.test(target.nodeName)) {
      const RTL = To(element);
      const arrowKeyNext = !RTL ? Ze : Ye;
      const arrowKeyPrev = !RTL ? Ye : Ze;
      // istanbul ignore else @preserve
      if (code === arrowKeyPrev) self.prev();
      else if (code === arrowKeyNext) self.next();
    }
  };
  function carouselDragHandler(e2) {
    const { target } = e2;
    const self = getCarouselInstance(this);
    // istanbul ignore next @preserve
    if (self && self.isTouch && (self.indicator && !self.indicator.contains(target) || !self.controls.includes(target))) {
      e2.stopImmediatePropagation();
      e2.stopPropagation();
      e2.preventDefault();
    }
  }
  function carouselPointerDownHandler(e2) {
    const { target } = e2;
    const self = getCarouselInstance(this);
    // istanbul ignore else @preserve
    if (self && !self.isAnimating && !self.isTouch) {
      const { controls, indicators } = self;
      // istanbul ignore else @preserve
      if (![...controls, ...indicators].every(
        (el) => el === target || el.contains(target)
      )) {
        startX = e2.pageX;
        // istanbul ignore else @preserve
        if (this.contains(target)) {
          self.isTouch = true;
          toggleCarouselTouchHandlers(self, true);
        }
      }
    }
  }
  const carouselPointerMoveHandler = (e2) => {
    currentX = e2.pageX;
  };
  const carouselPointerUpHandler = (e2) => {
    var _a;
    const { target } = e2;
    const doc = d(target);
    const self = [...ue(carouselSelector, doc)].map((c) => getCarouselInstance(c)).find((i2) => i2.isTouch);
    // istanbul ignore else @preserve
    if (self) {
      const { element, index } = self;
      const RTL = To(element);
      endX = e2.pageX;
      self.isTouch = false;
      toggleCarouselTouchHandlers(self);
      if (!((_a = doc.getSelection()) == null ? void 0 : _a.toString().length) && element.contains(target) && Math.abs(startX - endX) > 120) {
        // istanbul ignore else @preserve
        if (currentX < startX) {
          self.to(index + (RTL ? -1 : 1));
        } else if (currentX > startX) {
          self.to(index + (RTL ? 1 : -1));
        }
      }
      startX = 0;
      currentX = 0;
      endX = 0;
    }
  };
  const activateCarouselIndicator = (self, index) => {
    const { indicators } = self;
    [...indicators].forEach((x2) => Fn(x2, activeClass));
    // istanbul ignore else @preserve
    if (self.indicators[index]) Bn(indicators[index], activeClass);
  };
  const toggleCarouselTouchHandlers = (self, add) => {
    const { element } = self;
    const action = add ? E$1 : r;
    action(
      d(element),
      Ot,
      carouselPointerMoveHandler,
      eo
    );
    action(
      d(element),
      Lt,
      carouselPointerUpHandler,
      eo
    );
  };
  const getActiveIndex = (self) => {
    const { slides, element } = self;
    const activeItem = Co(`.${carouselItem}.${activeClass}`, element);
    return l(activeItem) ? [...slides].indexOf(activeItem) : -1;
  };
  class Carousel extends BaseComponent {
    /**
     * @param target mostly a `.carousel` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      /**
       * Toggles all event listeners for the `Carousel` instance.
       *
       * @param add when `TRUE` event listeners are added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const { element, options, slides, controls, indicators } = this;
        const { touch, pause, interval, keyboard } = options;
        const action = add ? E$1 : r;
        if (pause && interval) {
          action(element, bt, carouselPauseHandler);
          action(element, ht, carouselResumeHandler);
        }
        if (touch && slides.length > 2) {
          action(
            element,
            Dt,
            carouselPointerDownHandler,
            eo
          );
          action(element, Ut, carouselDragHandler, { passive: false });
          action(element, Ve, carouselDragHandler, { passive: false });
        }
        // istanbul ignore else @preserve
        if (controls.length) {
          controls.forEach((arrow) => {
            // istanbul ignore else @preserve
            if (arrow) action(arrow, ft, carouselControlsHandler);
          });
        }
        // istanbul ignore else @preserve
        if (indicators.length) {
          indicators.forEach((indicator) => {
            action(indicator, ft, carouselIndicatorHandler);
          });
        }
        if (keyboard) {
          action(d(element), ut, carouselKeyHandler);
        }
      });
      const { element } = this;
      this.direction = To(element) ? "right" : "left";
      this.isTouch = false;
      this.slides = Io(carouselItem, element);
      const { slides } = this;
      if (slides.length >= 2) {
        const activeIndex = getActiveIndex(this);
        const transitionItem = [...slides].find(
          (s) => xo(s, `.${carouselItem}-next,.${carouselItem}-next`)
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
        this.indicator = Co(`.${carouselString}-indicators`, element);
        this.indicators = [
          ...this.indicator ? ue(`[${dataBsSlideTo}]`, this.indicator) : [],
          ...ue(
            `[${dataBsSlideTo}][${dataBsTarget}="#${element.id}"]`,
            doc
          )
        ].filter((c, i2, ar) => i2 === ar.indexOf(c));
        const { options } = this;
        this.options.interval = options.interval === true ? carouselDefaults.interval : options.interval;
        // istanbul ignore next @preserve - impossible to test
        if (transitionItem) {
          this.index = [...slides].indexOf(transitionItem);
        } else if (activeIndex < 0) {
          this.index = 0;
          Bn(slides[0], activeClass);
          if (this.indicators.length) activateCarouselIndicator(this, 0);
        }
        // istanbul ignore else @preserve
        if (this.indicators.length) activateCarouselIndicator(this, this.index);
        this._toggleEventListeners(true);
        if (options.interval) this.cycle();
      }
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
      return Hn(this.element, pausedClass);
    }
    /**
     * Check if instance is animating.
     */
    get isAnimating() {
      return Co(
        `.${carouselItem}-next,.${carouselItem}-prev`,
        this.element
      ) !== null;
    }
    // CAROUSEL PUBLIC METHODS
    // =======================
    /** Slide automatically through items. */
    cycle() {
      const { element, options, isPaused, index } = this;
      so.clear(element, carouselString);
      if (isPaused) {
        so.clear(element, pausedClass);
        Fn(element, pausedClass);
      }
      so.set(
        element,
        () => {
          // istanbul ignore else @preserve
          if (this.element && !this.isPaused && !this.isTouch && vo(element)) {
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
      // istanbul ignore else @preserve
      if (!this.isPaused && options.interval) {
        Bn(element, pausedClass);
        so.set(
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
      // istanbul ignore else @preserve
      if (!this.isAnimating) {
        this.to(this.index + 1);
      }
    }
    /** Slide to the previous item. */
    prev() {
      // istanbul ignore else @preserve
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
      const RTL = To(element);
      let next = idx;
      if (!this.isAnimating && activeItem !== next && !so.get(element, dataBsSlide)) {
        // istanbul ignore else @preserve
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
        T(carouselSlideEvent, eventProperties);
        T(carouselSlidEvent, eventProperties);
        K(element, carouselSlideEvent);
        if (!carouselSlideEvent.defaultPrevented) {
          this.index = next;
          activateCarouselIndicator(this, next);
          if (re(slides[next]) && Hn(element, "slide")) {
            so.set(
              element,
              () => {
                Bn(slides[next], `${carouselItem}-${orientation}`);
                no(slides[next]);
                Bn(slides[next], `${carouselItem}-${directionClass}`);
                Bn(slides[activeItem], `${carouselItem}-${directionClass}`);
                qn(
                  slides[next],
                  () => this.slides && this.slides.length && carouselTransitionEndHandler(this)
                );
              },
              0,
              dataBsSlide
            );
          } else {
            Bn(slides[next], activeClass);
            Fn(slides[activeItem], activeClass);
            so.set(
              element,
              () => {
                so.clear(element, dataBsSlide);
                // istanbul ignore else @preserve
                if (element && options.interval && !this.isPaused) {
                  this.cycle();
                }
                K(element, carouselSlidEvent);
              },
              0,
              dataBsSlide
            );
          }
        }
      }
    }
    /** Remove `Carousel` component from target. */
    dispose() {
      const { isAnimating } = this;
      const clone = {
        ...this,
        isAnimating
      };
      this._toggleEventListeners();
      super.dispose();
      // istanbul ignore next @preserve - impossible to test
      if (clone.isAnimating) {
        qn(clone.slides[clone.index], () => {
          carouselTransitionEndHandler(clone);
        });
      }
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
  const getCollapseInstance = (element) => Rn(element, collapseComponent);
  const collapseInitCallback = (element) => new Collapse(element);
  const showCollapseEvent = to(`show.bs.${collapseString}`);
  const shownCollapseEvent = to(`shown.bs.${collapseString}`);
  const hideCollapseEvent = to(`hide.bs.${collapseString}`);
  const hiddenCollapseEvent = to(`hidden.bs.${collapseString}`);
  const expandCollapse = (self) => {
    const { element, parent, triggers } = self;
    K(element, showCollapseEvent);
    if (!showCollapseEvent.defaultPrevented) {
      so.set(element, _t, 17);
      if (parent) so.set(parent, _t, 17);
      Bn(element, collapsingClass);
      Fn(element, collapseString);
      oo(element, { height: `${element.scrollHeight}px` });
      qn(element, () => {
        so.clear(element);
        if (parent) so.clear(parent);
        triggers.forEach((btn) => In(btn, Ae, "true"));
        Fn(element, collapsingClass);
        Bn(element, collapseString);
        Bn(element, showClass);
        oo(element, { height: "" });
        K(element, shownCollapseEvent);
      });
    }
  };
  const collapseContent = (self) => {
    const { element, parent, triggers } = self;
    K(element, hideCollapseEvent);
    if (!hideCollapseEvent.defaultPrevented) {
      so.set(element, _t, 17);
      if (parent) so.set(parent, _t, 17);
      oo(element, { height: `${element.scrollHeight}px` });
      Fn(element, collapseString);
      Fn(element, showClass);
      Bn(element, collapsingClass);
      no(element);
      oo(element, { height: "0px" });
      qn(element, () => {
        so.clear(element);
        // istanbul ignore else @preserve
        if (parent) so.clear(parent);
        triggers.forEach((btn) => In(btn, Ae, "false"));
        Fn(element, collapsingClass);
        Bn(element, collapseString);
        oo(element, { height: "" });
        K(element, hiddenCollapseEvent);
      });
    }
  };
  const collapseClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && Ee(target, collapseToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getCollapseInstance(element);
    // istanbul ignore else @preserve
    if (self) self.toggle();
    if (trigger && trigger.tagName === "A") e2.preventDefault();
  };
  class Collapse extends BaseComponent {
    /**
     * @param target and `Element` that matches the selector
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      /**
       * Toggles on/off the event listener(s) of the `Collapse` instance.
       *
       * @param add when `true`, the event listener is added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        const { triggers } = this;
        // istanbul ignore else @preserve
        if (triggers.length) {
          triggers.forEach(
            (btn) => action(btn, ft, collapseClickHandler)
          );
        }
      });
      const { element, options } = this;
      const doc = d(element);
      this.triggers = [...ue(collapseToggleSelector, doc)].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.parent = l(options.parent) ? options.parent : M(options.parent) ? getTargetElement(element) || Co(options.parent, doc) : null;
      this._toggleEventListeners(true);
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
    /** Hides the collapse. */
    hide() {
      const { triggers, element } = this;
      // istanbul ignore else @preserve
      if (!so.get(element)) {
        collapseContent(this);
        // istanbul ignore else @preserve
        if (triggers.length) {
          triggers.forEach((btn) => Bn(btn, `${collapseString}d`));
        }
      }
    }
    /** Shows the collapse. */
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
      if ((!parent || !so.get(parent)) && !so.get(element)) {
        if (activeCollapseInstance && activeCollapse !== element) {
          collapseContent(activeCollapseInstance);
          activeCollapseInstance.triggers.forEach((btn) => {
            Bn(btn, `${collapseString}d`);
          });
        }
        expandCollapse(this);
        // istanbul ignore else @preserve
        if (triggers.length) {
          triggers.forEach((btn) => Fn(btn, `${collapseString}d`));
        }
      }
    }
    /** Toggles the visibility of the collapse. */
    toggle() {
      if (!Hn(this.element, showClass)) this.show();
      else this.hide();
    }
    /** Remove the `Collapse` component from the target `Element`. */
    dispose() {
      this._toggleEventListeners();
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
    const parentAnchor = Ee(element, "A");
    return element.tagName === "A" && // anchor href starts with #
    te(element, "href") && j(element, "href").slice(-1) === "#" || // OR a child of an anchor with href starts with #
    parentAnchor && te(parentAnchor, "href") && j(parentAnchor, "href").slice(-1) === "#";
  };
  const [dropdownString, dropupString, dropstartString, dropendString] = dropdownMenuClasses;
  const dropdownSelector = `[${dataBsToggle}="${dropdownString}"]`;
  const getDropdownInstance = (element) => Rn(element, dropdownComponent);
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
  const showDropdownEvent = to(
    `show.bs.${dropdownString}`
  );
  const shownDropdownEvent = to(
    `shown.bs.${dropdownString}`
  );
  const hideDropdownEvent = to(
    `hide.bs.${dropdownString}`
  );
  const hiddenDropdownEvent = to(`hidden.bs.${dropdownString}`);
  const updatedDropdownEvent = to(`updated.bs.${dropdownString}`);
  const styleDropdown = (self) => {
    const { element, menu, parentElement, options } = self;
    const { offset } = options;
    // istanbul ignore else @preserve: this test requires a navbar
    if (g(menu, "position") !== "static") {
      const RTL = To(element);
      const menuEnd = Hn(menu, dropdownMenuEndClass);
      const resetProps = ["margin", "top", "bottom", "left", "right"];
      resetProps.forEach((p) => {
        const style = {};
        style[p] = "";
        oo(menu, style);
      });
      let positionClass = dropdownMenuClasses.find(
        (c) => Hn(parentElement, c)
      ) || // istanbul ignore next @preserve: fallback position
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
      const { clientWidth, clientHeight } = k(element);
      const {
        left: targetLeft,
        top: targetTop,
        width: targetWidth,
        height: targetHeight
      } = h(element);
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
        T(dropdownPosition[positionClass], {
          top: "auto",
          bottom: 0
        });
      }
      if (verticalClass.includes(positionClass) && (leftExceed || rightExceed)) {
        let posAjust = { left: "auto", right: "auto" };
        // istanbul ignore else @preserve
        if (!leftExceed && rightExceed && !RTL) {
          posAjust = { left: "auto", right: 0 };
        }
        // istanbul ignore else @preserve
        if (leftExceed && !rightExceed && RTL) {
          posAjust = { left: 0, right: "auto" };
        }
        // istanbul ignore else @preserve
        if (posAjust) {
          T(dropdownPosition[positionClass], posAjust);
        }
      }
      const margins = dropdownMargin[positionClass];
      oo(menu, {
        ...dropdownPosition[positionClass],
        margin: `${margins.map((x2) => x2 ? `${x2}px` : x2).join(" ")}`
      });
      if (verticalClass.includes(positionClass) && menuEnd) {
        // istanbul ignore else @preserve
        if (menuEnd) {
          const endAdjust = !RTL && leftExceed || RTL && rightExceed ? "menuStart" : "menuEnd";
          oo(menu, dropdownPosition[endAdjust]);
        }
      }
      K(parentElement, updatedDropdownEvent);
    }
  };
  const getMenuItems = (menu) => {
    return [...menu.children].map((c) => {
      if (c && menuFocusTags.includes(c.tagName)) return c;
      const { firstElementChild } = c;
      if (firstElementChild && menuFocusTags.includes(firstElementChild.tagName)) {
        return firstElementChild;
      }
      return null;
    }).filter((c) => c);
  };
  const toggleDropdownDismiss = (self) => {
    const { element, options } = self;
    const action = self.open ? E$1 : r;
    const doc = d(element);
    action(doc, ft, dropdownDismissHandler);
    action(doc, ot, dropdownDismissHandler);
    action(doc, ut, dropdownPreventScroll);
    action(doc, dt, dropdownKeyHandler);
    // istanbul ignore else @preserve
    if (options.display === "dynamic") {
      [Ht, zt].forEach((ev) => {
        action(fo(element), ev, dropdownLayoutHandler, eo);
      });
    }
  };
  const getCurrentOpenDropdown = (element) => {
    const currentParent = [...dropdownMenuClasses, "btn-group", "input-group"].map(
      (c) => Io(`${c} ${showClass}`, d(element))
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
    // istanbul ignore else @preserve
    if (target && l(target)) {
      const element = getCurrentOpenDropdown(target);
      const self = element && getDropdownInstance(element);
      // istanbul ignore else @preserve
      if (self) {
        const { parentElement, menu } = self;
        const isForm = parentElement && parentElement.contains(target) && (target.tagName === "form" || Ee(target, "form") !== null);
        if ([ft, gt].includes(type) && isEmptyAnchor(target)) {
          e2.preventDefault();
        }
        // istanbul ignore else @preserve
        if (!isForm && type !== ot && target !== element && target !== menu) {
          self.hide();
        }
      }
    }
  };
  const dropdownClickHandler = (e2) => {
    const { target } = e2;
    const element = target && Ee(target, dropdownSelector);
    const self = element && getDropdownInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      e2.stopPropagation();
      self.toggle();
      // istanbul ignore else @preserve
      if (element && isEmptyAnchor(element)) e2.preventDefault();
    }
  };
  const dropdownPreventScroll = (e2) => {
    // istanbul ignore else @preserve
    if ([Je, Xe].includes(e2.code)) e2.preventDefault();
  };
  function dropdownKeyHandler(e2) {
    const { code } = e2;
    const element = getCurrentOpenDropdown(this);
    const self = element && getDropdownInstance(element);
    const { activeElement } = element && d(element);
    // istanbul ignore else @preserve
    if (self && activeElement) {
      const { menu, open } = self;
      const menuItems = getMenuItems(menu);
      if (menuItems && menuItems.length && [Je, Xe].includes(code)) {
        let idx = menuItems.indexOf(activeElement);
        // istanbul ignore else @preserve
        if (activeElement === element) {
          idx = 0;
        } else if (code === Xe) {
          idx = idx > 1 ? idx - 1 : 0;
        } else if (code === Je) {
          idx = idx < menuItems.length - 1 ? idx + 1 : idx;
        }
        // istanbul ignore else @preserve
        if (menuItems[idx]) Jn(menuItems[idx]);
      }
      if (sn === code && open) {
        self.toggle();
        Jn(element);
      }
    }
  }
  function dropdownLayoutHandler() {
    const element = getCurrentOpenDropdown(this);
    const self = element && getDropdownInstance(element);
    // istanbul ignore else @preserve
    if (self && self.open) styleDropdown(self);
  }
  class Dropdown extends BaseComponent {
    /**
     * @param target Element or string selector
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      /**
       * Toggles on/off the `click` event listener of the `Dropdown`.
       *
       * @param add when `true`, it will add the event listener
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        action(this.element, ft, dropdownClickHandler);
      });
      const { parentElement } = this.element;
      const [menu] = Io(
        dropdownMenuClass,
        parentElement
      );
      if (menu) {
        this.parentElement = parentElement;
        this.menu = menu;
        this._toggleEventListeners(true);
      }
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
      if (this.open) this.hide();
      else this.show();
    }
    /** Shows the dropdown menu to the user. */
    show() {
      const { element, open, menu, parentElement } = this;
      // istanbul ignore else @preserve
      if (!open) {
        const currentElement = getCurrentOpenDropdown(element);
        const currentInstance = currentElement && getDropdownInstance(currentElement);
        if (currentInstance) currentInstance.hide();
        [showDropdownEvent, shownDropdownEvent, updatedDropdownEvent].forEach(
          (e2) => {
            e2.relatedTarget = element;
          }
        );
        K(parentElement, showDropdownEvent);
        if (!showDropdownEvent.defaultPrevented) {
          Bn(menu, showClass);
          Bn(parentElement, showClass);
          In(element, Ae, "true");
          styleDropdown(this);
          this.open = !open;
          Jn(element);
          toggleDropdownDismiss(this);
          K(parentElement, shownDropdownEvent);
        }
      }
    }
    /** Hides the dropdown menu from the user. */
    hide() {
      const { element, open, menu, parentElement } = this;
      // istanbul ignore else @preserve
      if (open) {
        [hideDropdownEvent, hiddenDropdownEvent].forEach((e2) => {
          e2.relatedTarget = element;
        });
        K(parentElement, hideDropdownEvent);
        if (!hideDropdownEvent.defaultPrevented) {
          Fn(menu, showClass);
          Fn(parentElement, showClass);
          In(element, Ae, "false");
          this.open = !open;
          toggleDropdownDismiss(this);
          K(parentElement, hiddenDropdownEvent);
        }
      }
    }
    /** Removes the `Dropdown` component from the target element. */
    dispose() {
      if (this.open) this.hide();
      this._toggleEventListeners();
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
    ...Io(fixedTopClass, parent),
    ...Io(fixedBottomClass, parent),
    ...Io(stickyTopClass, parent),
    ...Io(positionStickyClass, parent),
    ...Io("is-fixed", parent)
  ];
  const resetScrollbar = (element) => {
    const bd = ao(element);
    oo(bd, {
      paddingRight: "",
      overflow: ""
    });
    const fixedItems = getFixedItems(bd);
    // istanbul ignore else @preserve
    if (fixedItems.length) {
      fixedItems.forEach((fixed) => {
        oo(fixed, {
          paddingRight: "",
          marginRight: ""
        });
      });
    }
  };
  const measureScrollbar = (element) => {
    const { clientWidth } = k(element);
    const { innerWidth } = fo(element);
    return Math.abs(innerWidth - clientWidth);
  };
  const setScrollbar = (element, overflow) => {
    const bd = ao(element);
    const bodyPad = parseInt(g(bd, "paddingRight"), 10);
    const isOpen = g(bd, "overflow") === "hidden";
    const sbWidth = isOpen && bodyPad ? 0 : measureScrollbar(element);
    const fixedItems = getFixedItems(bd);
    // istanbul ignore else @preserve
    if (overflow) {
      oo(bd, {
        overflow: "hidden",
        paddingRight: `${bodyPad + sbWidth}px`
      });
      // istanbul ignore else @preserve
      if (fixedItems.length) {
        fixedItems.forEach((fixed) => {
          const itemPadValue = g(fixed, "paddingRight");
          fixed.style.paddingRight = `${parseInt(itemPadValue, 10) + sbWidth}px`;
          // istanbul ignore else @preserve
          if ([stickyTopClass, positionStickyClass].some((c) => Hn(fixed, c))) {
            const itemMValue = g(fixed, "marginRight");
            fixed.style.marginRight = `${parseInt(itemMValue, 10) - sbWidth}px`;
          }
        });
      }
    }
  };
  const offcanvasString = "offcanvas";
  const popupContainer = ee({
    tagName: "div",
    className: "popup-container"
  });
  const appendPopup = (target, customContainer) => {
    const containerIsBody = i(customContainer) && customContainer.nodeName === "BODY";
    const lookup = i(customContainer) && !containerIsBody ? customContainer : popupContainer;
    const BODY = containerIsBody ? customContainer : ao(target);
    // istanbul ignore else @preserve
    if (i(target)) {
      if (lookup === popupContainer) {
        BODY.append(popupContainer);
      }
      lookup.append(target);
    }
  };
  const removePopup = (target, customContainer) => {
    const containerIsBody = i(customContainer) && customContainer.nodeName === "BODY";
    const lookup = i(customContainer) && !containerIsBody ? customContainer : popupContainer;
    // istanbul ignore else @preserve
    if (i(target)) {
      target.remove();
      if (lookup === popupContainer && !popupContainer.children.length) {
        popupContainer.remove();
      }
    }
  };
  const hasPopup = (target, customContainer) => {
    const lookup = i(customContainer) && customContainer.nodeName !== "BODY" ? customContainer : popupContainer;
    return i(target) && lookup.contains(target);
  };
  const backdropString = "backdrop";
  const modalBackdropClass = `${modalString}-${backdropString}`;
  const offcanvasBackdropClass = `${offcanvasString}-${backdropString}`;
  const modalActiveSelector = `.${modalString}.${showClass}`;
  const offcanvasActiveSelector = `.${offcanvasString}.${showClass}`;
  const overlay = ee("div");
  const getCurrentOpen = (element) => {
    return Co(
      `${modalActiveSelector},${offcanvasActiveSelector}`,
      d(element)
    );
  };
  const toggleOverlayType = (isModal) => {
    const targetClass = isModal ? modalBackdropClass : offcanvasBackdropClass;
    [modalBackdropClass, offcanvasBackdropClass].forEach((c) => {
      Fn(overlay, c);
    });
    Bn(overlay, targetClass);
  };
  const appendOverlay = (element, hasFade, isModal) => {
    toggleOverlayType(isModal);
    appendPopup(overlay, ao(element));
    if (hasFade) Bn(overlay, fadeClass);
  };
  const showOverlay = () => {
    if (!Hn(overlay, showClass)) {
      Bn(overlay, showClass);
      no(overlay);
    }
  };
  const hideOverlay = () => {
    Fn(overlay, showClass);
  };
  const removeOverlay = (element) => {
    if (!getCurrentOpen(element)) {
      Fn(overlay, fadeClass);
      removePopup(overlay, ao(element));
      resetScrollbar(element);
    }
  };
  const isVisible = (element) => {
    return l(element) && g(element, "visibility") !== "hidden" && element.offsetParent !== null;
  };
  const modalSelector = `.${modalString}`;
  const modalToggleSelector = `[${dataBsToggle}="${modalString}"]`;
  const modalDismissSelector = `[${dataBsDismiss}="${modalString}"]`;
  const modalStaticClass = `${modalString}-static`;
  const modalDefaults = {
    backdrop: true,
    keyboard: true
  };
  const getModalInstance = (element) => Rn(element, modalComponent);
  const modalInitCallback = (element) => new Modal(element);
  const showModalEvent = to(
    `show.bs.${modalString}`
  );
  const shownModalEvent = to(
    `shown.bs.${modalString}`
  );
  const hideModalEvent = to(
    `hide.bs.${modalString}`
  );
  const hiddenModalEvent = to(
    `hidden.bs.${modalString}`
  );
  const setModalScrollbar = (self) => {
    const { element } = self;
    const scrollbarWidth = measureScrollbar(element);
    const { clientHeight, scrollHeight } = k(element);
    const { clientHeight: modalHeight, scrollHeight: modalScrollHeight } = element;
    const modalOverflow = modalHeight !== modalScrollHeight;
    // istanbul ignore next @preserve: impossible to test?
    if (!modalOverflow && scrollbarWidth) {
      const pad = !To(element) ? "paddingRight" : "paddingLeft";
      const padStyle = { [pad]: `${scrollbarWidth}px` };
      oo(element, padStyle);
    }
    setScrollbar(element, modalOverflow || clientHeight !== scrollHeight);
  };
  const toggleModalDismiss = (self, add) => {
    const action = add ? E$1 : r;
    const { element, update } = self;
    action(element, ft, modalDismissHandler);
    action(fo(element), zt, update, eo);
    action(d(element), ut, modalKeyHandler);
  };
  const afterModalHide = (self) => {
    const { triggers, element, relatedTarget } = self;
    removeOverlay(element);
    oo(element, { paddingRight: "", display: "" });
    toggleModalDismiss(self);
    const focusElement = showModalEvent.relatedTarget || triggers.find(isVisible);
    // istanbul ignore else @preserve
    if (focusElement) Jn(focusElement);
    hiddenModalEvent.relatedTarget = relatedTarget;
    K(element, hiddenModalEvent);
    ro(element);
  };
  const afterModalShow = (self) => {
    const { element, relatedTarget } = self;
    Jn(element);
    toggleModalDismiss(self, true);
    shownModalEvent.relatedTarget = relatedTarget;
    K(element, shownModalEvent);
    ro(element);
  };
  const beforeModalShow = (self) => {
    const { element, hasFade } = self;
    oo(element, { display: "block" });
    setModalScrollbar(self);
    // istanbul ignore else @preserve
    if (!getCurrentOpen(element)) {
      oo(ao(element), { overflow: "hidden" });
    }
    Bn(element, showClass);
    zn(element, X);
    In(element, Te, "true");
    if (hasFade) qn(element, () => afterModalShow(self));
    else afterModalShow(self);
  };
  const beforeModalHide = (self) => {
    const { element, options, hasFade } = self;
    if (options.backdrop && hasFade && Hn(overlay, showClass) && !getCurrentOpen(element)) {
      hideOverlay();
      qn(overlay, () => afterModalHide(self));
    } else {
      afterModalHide(self);
    }
  };
  const modalClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && Ee(target, modalToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getModalInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      // istanbul ignore else @preserve
      if (trigger && trigger.tagName === "A") e2.preventDefault();
      self.relatedTarget = trigger;
      self.toggle();
    }
  };
  const modalKeyHandler = ({ code, target }) => {
    const element = Co(modalActiveSelector, d(target));
    const self = element && getModalInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      const { options } = self;
      // istanbul ignore else @preserve
      if (options.keyboard && code === sn && // the keyboard option is enabled and the key is 27
      Hn(element, showClass)) {
        self.relatedTarget = null;
        self.hide();
      }
    }
  };
  const modalDismissHandler = (e2) => {
    var _a, _b;
    const { currentTarget } = e2;
    const self = currentTarget && getModalInstance(currentTarget);
    // istanbul ignore else @preserve
    if (self && currentTarget && !so.get(currentTarget)) {
      const { options, isStatic, modalDialog } = self;
      const { backdrop } = options;
      const { target } = e2;
      const selectedText = (_b = (_a = d(currentTarget)) == null ? void 0 : _a.getSelection()) == null ? void 0 : _b.toString().length;
      const targetInsideDialog = modalDialog.contains(target);
      const dismiss = target && Ee(target, modalDismissSelector);
      // istanbul ignore else @preserve
      if (isStatic && !targetInsideDialog) {
        so.set(
          currentTarget,
          () => {
            Bn(currentTarget, modalStaticClass);
            qn(modalDialog, () => staticTransitionEnd(self));
          },
          17
        );
      } else if (dismiss || !selectedText && !isStatic && !targetInsideDialog && backdrop) {
        self.relatedTarget = dismiss || null;
        self.hide();
        e2.preventDefault();
      }
    }
  };
  const staticTransitionEnd = (self) => {
    const { element, modalDialog } = self;
    const duration = (re(modalDialog) || 0) + 17;
    Fn(element, modalStaticClass);
    so.set(element, () => so.clear(element), duration);
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
        // istanbul ignore else @preserve
        if (Hn(this.element, showClass)) setModalScrollbar(this);
      });
      /**
       * Toggles on/off the `click` event listener of the `Modal` instance.
       *
       * @param add when `true`, event listener(s) is/are added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        const { triggers } = this;
        // istanbul ignore else @preserve
        if (triggers.length) {
          triggers.forEach(
            (btn) => action(btn, ft, modalClickHandler)
          );
        }
      });
      const { element } = this;
      const modalDialog = Co(`.${modalString}-dialog`, element);
      // istanbul ignore else @preserve
      if (modalDialog) {
        this.modalDialog = modalDialog;
        this.triggers = [
          ...ue(modalToggleSelector, d(element))
        ].filter(
          (btn) => getTargetElement(btn) === element
        );
        this.isStatic = this.options.backdrop === "static";
        this.hasFade = Hn(element, fadeClass);
        this.relatedTarget = null;
        this._toggleEventListeners(true);
      }
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
      if (Hn(this.element, showClass)) this.hide();
      else this.show();
    }
    /** Shows the modal to the user. */
    show() {
      const { element, options, hasFade, relatedTarget } = this;
      const { backdrop } = options;
      let overlayDelay = 0;
      // istanbul ignore else @preserve
      if (!Hn(element, showClass)) {
        showModalEvent.relatedTarget = relatedTarget || void 0;
        K(element, showModalEvent);
        if (!showModalEvent.defaultPrevented) {
          const currentOpen = getCurrentOpen(element);
          // istanbul ignore else @preserve
          if (currentOpen && currentOpen !== element) {
            const that = getModalInstance(currentOpen) || // istanbul ignore next @preserve
            Rn(
              currentOpen,
              offcanvasComponent
            );
            // istanbul ignore else @preserve
            if (that) that.hide();
          }
          if (backdrop) {
            if (!hasPopup(overlay)) {
              appendOverlay(element, hasFade, true);
            } else {
              toggleOverlayType(true);
            }
            overlayDelay = re(overlay);
            showOverlay();
            setTimeout(() => beforeModalShow(this), overlayDelay);
          } else {
            beforeModalShow(this);
            // istanbul ignore else @preserve
            if (currentOpen && Hn(overlay, showClass)) {
              hideOverlay();
            }
          }
        }
      }
    }
    /** Hide the modal from the user. */
    hide() {
      const { element, hasFade, relatedTarget } = this;
      // istanbul ignore else @preserve
      if (Hn(element, showClass)) {
        hideModalEvent.relatedTarget = relatedTarget || void 0;
        K(element, hideModalEvent);
        // istanbul ignore else @preserve
        if (!hideModalEvent.defaultPrevented) {
          Fn(element, showClass);
          In(element, X, "true");
          zn(element, Te);
          if (hasFade) {
            qn(element, () => beforeModalHide(this));
          } else {
            beforeModalHide(this);
          }
        }
      }
    }
    /** Removes the `Modal` component from target element. */
    dispose() {
      const clone = { ...this };
      const { modalDialog, hasFade } = clone;
      const callback = () => setTimeout(() => super.dispose(), 17);
      this.hide();
      this._toggleEventListeners();
      if (hasFade) {
        qn(modalDialog, callback);
      } else {
        callback();
      }
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
  const getOffcanvasInstance = (element) => Rn(element, offcanvasComponent);
  const offcanvasInitCallback = (element) => new Offcanvas(element);
  const showOffcanvasEvent = to(`show.bs.${offcanvasString}`);
  const shownOffcanvasEvent = to(`shown.bs.${offcanvasString}`);
  const hideOffcanvasEvent = to(`hide.bs.${offcanvasString}`);
  const hiddenOffcanvasEvent = to(`hidden.bs.${offcanvasString}`);
  const setOffCanvasScrollbar = (self) => {
    const { element } = self;
    const { clientHeight, scrollHeight } = k(element);
    setScrollbar(element, clientHeight !== scrollHeight);
  };
  const toggleOffCanvasDismiss = (self, add) => {
    const action = add ? E$1 : r;
    const doc = d(self.element);
    action(doc, ut, offcanvasKeyDismissHandler);
    action(doc, ft, offcanvasDismissHandler);
  };
  const beforeOffcanvasShow = (self) => {
    const { element, options } = self;
    // istanbul ignore else @preserve
    if (!options.scroll) {
      setOffCanvasScrollbar(self);
      oo(ao(element), { overflow: "hidden" });
    }
    Bn(element, offcanvasTogglingClass);
    Bn(element, showClass);
    oo(element, { visibility: "visible" });
    qn(element, () => showOffcanvasComplete(self));
  };
  const beforeOffcanvasHide = (self) => {
    const { element, options } = self;
    const currentOpen = getCurrentOpen(element);
    element.blur();
    if (!currentOpen && options.backdrop && Hn(overlay, showClass)) {
      hideOverlay();
    }
    qn(element, () => hideOffcanvasComplete(self));
  };
  const offcanvasTriggerHandler = (e2) => {
    const trigger = Ee(e2.target, offcanvasToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getOffcanvasInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      self.relatedTarget = trigger;
      self.toggle();
      // istanbul ignore else @preserve
      if (trigger && trigger.tagName === "A") {
        e2.preventDefault();
      }
    }
  };
  const offcanvasDismissHandler = (e2) => {
    const { target } = e2;
    const element = Co(
      offcanvasActiveSelector,
      d(target)
    );
    const offCanvasDismiss = Co(
      offcanvasDismissSelector,
      element
    );
    const self = element && getOffcanvasInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      const { options, triggers } = self;
      const { backdrop } = options;
      const trigger = Ee(target, offcanvasToggleSelector);
      const selection = d(element).getSelection();
      // istanbul ignore else: a filter is required here @preserve
      if (!overlay.contains(target) || backdrop !== "static") {
        // istanbul ignore else @preserve
        if (!(selection && selection.toString().length) && (!element.contains(target) && backdrop && // istanbul ignore next @preserve
        (!trigger || triggers.includes(target)) || offCanvasDismiss && offCanvasDismiss.contains(target))) {
          self.relatedTarget = offCanvasDismiss && offCanvasDismiss.contains(target) ? offCanvasDismiss : null;
          self.hide();
        }
        // istanbul ignore next @preserve
        if (trigger && trigger.tagName === "A") e2.preventDefault();
      }
    }
  };
  const offcanvasKeyDismissHandler = ({ code, target }) => {
    const element = Co(
      offcanvasActiveSelector,
      d(target)
    );
    const self = element && getOffcanvasInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      // istanbul ignore else @preserve
      if (self.options.keyboard && code === sn) {
        self.relatedTarget = null;
        self.hide();
      }
    }
  };
  const showOffcanvasComplete = (self) => {
    const { element } = self;
    Fn(element, offcanvasTogglingClass);
    zn(element, X);
    In(element, Te, "true");
    In(element, "role", "dialog");
    K(element, shownOffcanvasEvent);
    toggleOffCanvasDismiss(self, true);
    Jn(element);
    ro(element);
  };
  const hideOffcanvasComplete = (self) => {
    const { element, triggers } = self;
    In(element, X, "true");
    zn(element, Te);
    zn(element, "role");
    oo(element, { visibility: "" });
    const visibleTrigger = showOffcanvasEvent.relatedTarget || triggers.find(isVisible);
    // istanbul ignore else @preserve
    if (visibleTrigger) Jn(visibleTrigger);
    removeOverlay(element);
    K(element, hiddenOffcanvasEvent);
    Fn(element, offcanvasTogglingClass);
    ro(element);
    if (!getCurrentOpen(element)) {
      toggleOffCanvasDismiss(self);
    }
  };
  class Offcanvas extends BaseComponent {
    /**
     * @param target usually an `.offcanvas` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      /**
       * Toggles on/off the `click` event listeners.
       *
       * @param self the `Offcanvas` instance
       * @param add when *true*, listeners are added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        this.triggers.forEach(
          (btn) => action(btn, ft, offcanvasTriggerHandler)
        );
      });
      const { element } = this;
      this.triggers = [
        ...ue(offcanvasToggleSelector, d(element))
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.relatedTarget = null;
      this._toggleEventListeners(true);
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
      if (Hn(this.element, showClass)) this.hide();
      else this.show();
    }
    /** Shows the offcanvas to the user. */
    show() {
      const { element, options, relatedTarget } = this;
      let overlayDelay = 0;
      if (!Hn(element, showClass)) {
        showOffcanvasEvent.relatedTarget = relatedTarget || void 0;
        shownOffcanvasEvent.relatedTarget = relatedTarget || void 0;
        K(element, showOffcanvasEvent);
        if (!showOffcanvasEvent.defaultPrevented) {
          const currentOpen = getCurrentOpen(element);
          if (currentOpen && currentOpen !== element) {
            const that = getOffcanvasInstance(currentOpen) || // istanbul ignore next @preserve
            Rn(
              currentOpen,
              modalComponent
            );
            // istanbul ignore else @preserve
            if (that) that.hide();
          }
          if (options.backdrop) {
            if (!hasPopup(overlay)) {
              appendOverlay(element, true);
            } else {
              toggleOverlayType();
            }
            overlayDelay = re(overlay);
            showOverlay();
            setTimeout(() => beforeOffcanvasShow(this), overlayDelay);
          } else {
            beforeOffcanvasShow(this);
            // istanbul ignore next @preserve - this test was done on Modal
            if (currentOpen && Hn(overlay, showClass)) {
              hideOverlay();
            }
          }
        }
      }
    }
    /** Hides the offcanvas from the user. */
    hide() {
      const { element, relatedTarget } = this;
      if (Hn(element, showClass)) {
        hideOffcanvasEvent.relatedTarget = relatedTarget || void 0;
        hiddenOffcanvasEvent.relatedTarget = relatedTarget || void 0;
        K(element, hideOffcanvasEvent);
        if (!hideOffcanvasEvent.defaultPrevented) {
          Bn(element, offcanvasTogglingClass);
          Fn(element, showClass);
          beforeOffcanvasHide(this);
        }
      }
    }
    /** Removes the `Offcanvas` from the target element. */
    dispose() {
      const { element } = this;
      const isOpen = Hn(element, showClass);
      const callback = () => setTimeout(() => super.dispose(), 1);
      this.hide();
      this._toggleEventListeners();
      if (isOpen) {
        qn(element, callback);
        // istanbul ignore next @preserve
      } else {
        callback();
      }
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
    // istanbul ignore else @preserve
    if (tooltip) {
      const tipPositions = { ...tipClassPositions };
      const RTL = To(element);
      oo(tooltip, {
        // top: '0px', left: '0px', right: '', bottom: '',
        top: "",
        left: "",
        right: "",
        bottom: ""
      });
      const isPopover = self.name === popoverComponent;
      const { offsetWidth: tipWidth, offsetHeight: tipHeight } = tooltip;
      const { clientWidth: htmlcw, clientHeight: htmlch, offsetWidth: htmlow } = k(element);
      let { placement } = options;
      const { clientWidth: parentCWidth, offsetWidth: parentOWidth } = container;
      const parentPosition = g(
        container,
        "position"
      );
      const fixedParent = parentPosition === "fixed";
      const scrollbarWidth = fixedParent ? Math.abs(parentCWidth - parentOWidth) : Math.abs(htmlcw - htmlow);
      const leftBoundry = RTL && fixedParent ? scrollbarWidth : 0;
      const rightBoundry = htmlcw - (!RTL ? scrollbarWidth : 0) - 1;
      const {
        width: elemWidth,
        height: elemHeight,
        left: elemRectLeft,
        right: elemRectRight,
        top: elemRectTop
      } = h(element, true);
      const { x: x2, y } = {
        x: elemRectLeft,
        y: elemRectTop
      };
      oo(arrow, {
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
          tipPositions[placement]
        );
      }
      // istanbul ignore else @preserve
      if (horizontals.includes(placement)) {
        if (placement === "left") {
          leftPosition = x2 - tipWidth - (isPopover ? arrowWidth : 0);
        } else {
          leftPosition = x2 + elemWidth + (isPopover ? arrowWidth : 0);
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
          arrowLeft = x2 + elemWidth / 2 - arrowAdjust;
        } else if (rightExceed) {
          leftPosition = "auto";
          rightPosition = 0;
          arrowRight = elemWidth / 2 + rightBoundry - elemRectRight - arrowAdjust;
        } else {
          leftPosition = x2 - tipWidth / 2 + elemWidth / 2;
          arrowLeft = tipWidth / 2 - arrowAdjust;
        }
      }
      oo(tooltip, {
        top: `${topPosition}px`,
        bottom: bottomPosition === "" ? "" : `${bottomPosition}px`,
        left: leftPosition === "auto" ? leftPosition : `${leftPosition}px`,
        right: rightPosition !== "" ? `${rightPosition}px` : ""
      });
      // istanbul ignore else @preserve
      if (l(arrow)) {
        if (arrowTop !== "") {
          arrow.style.top = `${arrowTop}px`;
        }
        if (arrowLeft !== "") {
          arrow.style.left = `${arrowLeft}px`;
        } else if (arrowRight !== "") {
          arrow.style.right = `${arrowRight}px`;
        }
      }
      const updatedTooltipEvent = to(
        `updated.bs.${ae(self.name)}`
      );
      K(element, updatedTooltipEvent);
    }
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
    // istanbul ignore else @preserve
    if (M(content) && content.length) {
      let dirty = content.trim();
      if (ho(sanitizeFn)) dirty = sanitizeFn(dirty);
      const domParser = new DOMParser();
      const tempDocument = domParser.parseFromString(dirty, "text/html");
      element.append(...[...tempDocument.body.childNodes]);
    } else if (l(content)) {
      element.append(content);
    } else if (Mo(content) || ge(content) && content.every(i)) {
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
    if (To(element)) {
      tipPositions.left = "end";
      tipPositions.right = "start";
    }
    const placementClass = `bs-${tipString}-${tipPositions[placement]}`;
    let tooltipTemplate;
    if (l(template)) {
      tooltipTemplate = template;
    } else {
      const htmlMarkup = ee("div");
      setHtml(htmlMarkup, template, sanitizeFn);
      tooltipTemplate = htmlMarkup.firstChild;
    }
    self.tooltip = l(tooltipTemplate) ? tooltipTemplate.cloneNode(true) : void 0;
    const { tooltip } = self;
    // istanbul ignore else @preserve
    if (tooltip) {
      In(tooltip, "id", id);
      In(tooltip, "role", tooltipString);
      const bodyClass = isTooltip ? `${tooltipString}-inner` : `${popoverString}-body`;
      const tooltipHeader = isTooltip ? null : Co(`.${popoverString}-header`, tooltip);
      const tooltipBody = Co(`.${bodyClass}`, tooltip);
      self.arrow = Co(`.${tipString}-arrow`, tooltip);
      const { arrow } = self;
      if (l(title)) titleParts = [title.cloneNode(true)];
      else {
        const tempTitle = ee("div");
        setHtml(tempTitle, title, sanitizeFn);
        titleParts = [...[...tempTitle.childNodes]];
      }
      if (l(content)) contentParts = [content.cloneNode(true)];
      else {
        const tempContent = ee("div");
        setHtml(tempContent, content, sanitizeFn);
        contentParts = [...[...tempContent.childNodes]];
      }
      if (dismissible) {
        if (title) {
          if (l(btnClose)) {
            titleParts = [...titleParts, btnClose.cloneNode(true)];
          } else {
            const tempBtn = ee("div");
            setHtml(tempBtn, btnClose, sanitizeFn);
            titleParts = [...titleParts, tempBtn.firstChild];
          }
        } else {
          // istanbul ignore else @preserve
          if (tooltipHeader) tooltipHeader.remove();
          if (l(btnClose)) {
            contentParts = [...contentParts, btnClose.cloneNode(true)];
          } else {
            const tempBtn = ee("div");
            setHtml(tempBtn, btnClose, sanitizeFn);
            contentParts = [...contentParts, tempBtn.firstChild];
          }
        }
      }
      // istanbul ignore else @preserve
      if (!isTooltip) {
        // istanbul ignore else @preserve
        if (title && tooltipHeader) {
          setHtml(tooltipHeader, titleParts, sanitizeFn);
        }
        // istanbul ignore else @preserve
        if (content && tooltipBody) {
          setHtml(tooltipBody, contentParts, sanitizeFn);
        }
        self.btn = Co(".btn-close", tooltip) || void 0;
      } else if (title && tooltipBody) setHtml(tooltipBody, title, sanitizeFn);
      Bn(tooltip, "position-fixed");
      Bn(arrow, "position-absolute");
      // istanbul ignore else @preserve
      if (!Hn(tooltip, tipString)) Bn(tooltip, tipString);
      // istanbul ignore else @preserve
      if (animation && !Hn(tooltip, fadeClass)) {
        Bn(tooltip, fadeClass);
      }
      // istanbul ignore else @preserve
      if (customClass && !Hn(tooltip, customClass)) {
        Bn(tooltip, customClass);
      }
      // istanbul ignore else @preserve
      if (!Hn(tooltip, placementClass)) Bn(tooltip, placementClass);
    }
  };
  const getElementContainer = (element) => {
    const majorBlockTags = ["HTML", "BODY"];
    const containers = [];
    let { parentNode } = element;
    while (parentNode && !majorBlockTags.includes(parentNode.nodeName)) {
      parentNode = lo(parentNode);
      // istanbul ignore else @preserve
      if (!(pe(parentNode) || Do(parentNode))) {
        containers.push(parentNode);
      }
    }
    return containers.find((c, i2) => {
      if (g(c, "position") !== "relative" && containers.slice(i2 + 1).every(
        (r2) => g(r2, "position") === "static"
      )) {
        return c;
      }
      return null;
    }) || // istanbul ignore next: optional guard
    d(element).body;
  };
  const tooltipSelector = `[${dataBsToggle}="${tooltipString}"],[data-tip="${tooltipString}"]`;
  const titleAttr = "title";
  let getTooltipInstance = (element) => Rn(element, tooltipComponent);
  const tooltipInitCallback = (element) => new Tooltip(element);
  const removeTooltip = (self) => {
    const { element, tooltip, container, offsetParent } = self;
    zn(element, we);
    removePopup(
      tooltip,
      container === offsetParent ? container : offsetParent
    );
  };
  const hasTip = (self) => {
    const { tooltip, container, offsetParent } = self;
    return tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent);
  };
  const disposeTooltipComplete = (self, callback) => {
    const { element } = self;
    self._toggleEventListeners();
    // istanbul ignore else @preserve
    if (te(element, dataOriginalTitle) && self.name === tooltipComponent) {
      toggleTooltipTitle(self);
    }
    // istanbul ignore else @preserve
    if (callback) callback();
  };
  const toggleTooltipAction = (self, add) => {
    const action = add ? E$1 : r;
    const { element } = self;
    action(
      d(element),
      Ut,
      self.handleTouch,
      eo
    );
    [Ht, zt].forEach((ev) => {
      action(fo(element), ev, self.update, eo);
    });
  };
  const tooltipShownAction = (self) => {
    const { element } = self;
    const shownTooltipEvent = to(
      `shown.bs.${ae(self.name)}`
    );
    toggleTooltipAction(self, true);
    K(element, shownTooltipEvent);
    so.clear(element, "in");
  };
  const tooltipHiddenAction = (self) => {
    const { element } = self;
    const hiddenTooltipEvent = to(
      `hidden.bs.${ae(self.name)}`
    );
    toggleTooltipAction(self);
    removeTooltip(self);
    K(element, hiddenTooltipEvent);
    so.clear(element, "out");
  };
  const toggleTooltipOpenHandlers = (self, add) => {
    const action = add ? E$1 : r;
    const { element, container, offsetParent } = self;
    const { offsetHeight, scrollHeight } = container;
    const parentModal = Ee(element, `.${modalString}`);
    const parentOffcanvas = Ee(element, `.${offcanvasString}`);
    // istanbul ignore else @preserve
    const win = fo(element);
    const overflow = offsetHeight !== scrollHeight;
    const scrollTarget = container === offsetParent && overflow ? container : win;
    action(scrollTarget, zt, self.update, eo);
    action(scrollTarget, Ht, self.update, eo);
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
    In(
      element,
      titleAtt[content ? 0 : 1],
      content || j(element, titleAtt[0]) || // istanbul ignore next @preserve
      ""
    );
    zn(element, titleAtt[content ? 1 : 0]);
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
      /** Handles the focus event on iOS. */
      // istanbul ignore next @preserve - impossible to test without Apple device
      __publicField(this, "handleFocus", () => Jn(this.element));
      /** Shows the tooltip. */
      __publicField(this, "handleShow", () => this.show());
      /** Hides the tooltip. */
      __publicField(this, "handleHide", () => this.hide());
      /** Updates the tooltip position. */
      __publicField(this, "update", () => {
        styleTip(this);
      });
      /** Toggles the tooltip visibility. */
      __publicField(this, "toggle", () => {
        const { tooltip } = this;
        if (tooltip && !hasTip(this)) this.show();
        else this.hide();
      });
      /**
       * Handles the `touchstart` event listener for `Tooltip`
       *
       * @this {Tooltip}
       * @param {TouchEvent} e the `Event` object
       */
      __publicField(this, "handleTouch", ({ target }) => {
        const { tooltip, element } = this;
        // istanbul ignore if @preserve
        if (tooltip && tooltip.contains(target) || target === element || target && element.contains(target)) ;
        else {
          this.hide();
        }
      });
      /**
       * Toggles on/off the `Tooltip` event listeners.
       *
       * @param add when `true`, event listeners are added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        const { element, options, btn } = this;
        const { trigger } = options;
        const isPopover = this.name !== tooltipComponent;
        const dismissible = isPopover && options.dismissible ? true : false;
        // istanbul ignore else @preserve
        if (!trigger.includes("manual")) {
          this.enabled = !!add;
          const triggerOptions = trigger.split(" ");
          triggerOptions.forEach((tr) => {
            // istanbul ignore else @preserve
            if (tr === Et) {
              action(element, gt, this.handleShow);
              action(element, bt, this.handleShow);
              // istanbul ignore else @preserve
              if (!dismissible) {
                action(element, ht, this.handleHide);
                action(
                  d(element),
                  Ut,
                  this.handleTouch,
                  eo
                );
              }
            } else if (tr === ft) {
              action(element, tr, !dismissible ? this.toggle : this.handleShow);
            } else if (tr === ot) {
              action(element, st, this.handleShow);
              // istanbul ignore else @preserve
              if (!dismissible) action(element, ct, this.handleHide);
              // istanbul ignore else @preserve
              if (An) {
                action(element, ft, this.handleFocus);
              }
            }
            // istanbul ignore else @preserve
            if (dismissible && btn) {
              action(btn, ft, this.handleHide);
            }
          });
        }
      });
      const { element } = this;
      const isTooltip = this.name === tooltipComponent;
      const tipString = isTooltip ? tooltipString : popoverString;
      const tipComponent = isTooltip ? tooltipComponent : popoverComponent;
      // istanbul ignore next @preserve: this is to set Popover too
      getTooltipInstance = (elem) => Rn(elem, tipComponent);
      this.enabled = true;
      this.id = `${tipString}-${me(element, tipString)}`;
      const { options } = this;
      if (!(!options.title && isTooltip || !isTooltip && !options.content)) {
        T(tooltipDefaults, { titleAttr: "" });
        // istanbul ignore else @preserve
        if (te(element, titleAttr) && isTooltip && typeof options.title === "string") {
          toggleTooltipTitle(this, options.title);
        }
        this.container = getElementContainer(element);
        this.offsetParent = ["sticky", "fixed"].some(
          (position) => g(this.container, "position") === position
        ) ? this.container : d(this.element).body;
        createTip(this);
        this._toggleEventListeners(true);
      }
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
    show() {
      const { options, tooltip, element, container, offsetParent, id } = this;
      const { animation } = options;
      const outTimer = so.get(element, "out");
      const tipContainer = container === offsetParent ? container : offsetParent;
      so.clear(element, "out");
      if (tooltip && !outTimer && !hasTip(this)) {
        so.set(
          element,
          () => {
            const showTooltipEvent = to(
              `show.bs.${ae(this.name)}`
            );
            K(element, showTooltipEvent);
            // istanbul ignore else @preserve
            if (!showTooltipEvent.defaultPrevented) {
              appendPopup(tooltip, tipContainer);
              In(element, we, `#${id}`);
              this.update();
              toggleTooltipOpenHandlers(this, true);
              // istanbul ignore else @preserve
              if (!Hn(tooltip, showClass)) Bn(tooltip, showClass);
              // istanbul ignore else @preserve
              if (animation) {
                qn(tooltip, () => tooltipShownAction(this));
              } else tooltipShownAction(this);
            }
          },
          17,
          "in"
        );
      }
    }
    hide() {
      const { options, tooltip, element } = this;
      const { animation, delay } = options;
      so.clear(element, "in");
      // istanbul ignore else @preserve
      if (tooltip && hasTip(this)) {
        so.set(
          element,
          () => {
            const hideTooltipEvent = to(
              `hide.bs.${ae(this.name)}`
            );
            K(element, hideTooltipEvent);
            // istanbul ignore else @preserve
            if (!hideTooltipEvent.defaultPrevented) {
              this.update();
              Fn(tooltip, showClass);
              toggleTooltipOpenHandlers(this);
              // istanbul ignore else @preserve
              if (animation) {
                qn(tooltip, () => tooltipHiddenAction(this));
              } else tooltipHiddenAction(this);
            }
          },
          delay + 17,
          "out"
        );
      }
    }
    /** Enables the tooltip. */
    enable() {
      const { enabled } = this;
      // istanbul ignore else @preserve
      if (!enabled) {
        this._toggleEventListeners(true);
        this.enabled = !enabled;
      }
    }
    /** Disables the tooltip. */
    disable() {
      const { tooltip, enabled } = this;
      // istanbul ignore else @preserve
      if (enabled) {
        if (tooltip && hasTip(this)) this.hide();
        this._toggleEventListeners();
        this.enabled = !enabled;
      }
    }
    /** Toggles the `disabled` property. */
    toggleEnabled() {
      if (!this.enabled) this.enable();
      else this.disable();
    }
    /** Removes the `Tooltip` from the target element. */
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
        qn(tooltip, callback);
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
  const popoverDefaults = T({}, tooltipDefaults, {
    template: getTipTemplate(popoverString),
    content: "",
    dismissible: false,
    btnClose: '<button class="btn-close" aria-label="Close"></button>'
  });
  const getPopoverInstance = (element) => Rn(element, popoverComponent);
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
        super.show();
        const { options, btn } = this;
        // istanbul ignore else @preserve
        if (options.dismissible && btn) setTimeout(() => Jn(btn), 17);
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
  const getTabInstance = (element) => Rn(element, tabComponent);
  const tabInitCallback = (element) => new Tab(element);
  const showTabEvent = to(
    `show.bs.${tabString}`
  );
  const shownTabEvent = to(
    `shown.bs.${tabString}`
  );
  const hideTabEvent = to(
    `hide.bs.${tabString}`
  );
  const hiddenTabEvent = to(
    `hidden.bs.${tabString}`
  );
  const tabPrivate = /* @__PURE__ */ new Map();
  const triggerTabEnd = (self) => {
    const { tabContent, nav } = self;
    // istanbul ignore else @preserve
    if (tabContent && Hn(tabContent, collapsingClass)) {
      tabContent.style.height = "";
      Fn(tabContent, collapsingClass);
    }
    // istanbul ignore else @preserve
    if (nav) so.clear(nav);
  };
  const triggerTabShow = (self) => {
    const { element, tabContent, content: nextContent, nav } = self;
    const { tab } = l(nav) && tabPrivate.get(nav) || // istanbul ignore next @preserve
    { tab: null };
    // istanbul ignore else @preserve
    if (tabContent && nextContent && Hn(nextContent, fadeClass)) {
      const { currentHeight, nextHeight } = tabPrivate.get(element) || // istanbul ignore next @preserve
      { currentHeight: 0, nextHeight: 0 };
      // istanbul ignore else @preserve: vitest won't validate this branch
      if (currentHeight !== nextHeight) {
        setTimeout(() => {
          tabContent.style.height = `${nextHeight}px`;
          no(tabContent);
          qn(tabContent, () => triggerTabEnd(self));
        }, 50);
      } else {
        triggerTabEnd(self);
      }
    } else if (nav) so.clear(nav);
    shownTabEvent.relatedTarget = tab;
    K(element, shownTabEvent);
  };
  const triggerTabHide = (self) => {
    const { element, content: nextContent, tabContent, nav } = self;
    const { tab, content } = nav && tabPrivate.get(nav) || // istanbul ignore next @preserve
    { tab: null, content: null };
    let currentHeight = 0;
    // istanbul ignore else @preserve
    if (tabContent && nextContent && Hn(nextContent, fadeClass)) {
      [content, nextContent].forEach((c) => {
        // istanbul ignore else @preserve
        if (l(c)) Bn(c, "overflow-hidden");
      });
      currentHeight = l(content) ? content.scrollHeight : 0;
    }
    showTabEvent.relatedTarget = tab;
    hiddenTabEvent.relatedTarget = element;
    K(element, showTabEvent);
    // istanbul ignore else @preserve
    if (!showTabEvent.defaultPrevented) {
      // istanbul ignore else @preserve
      if (nextContent) Bn(nextContent, activeClass);
      // istanbul ignore else @preserve
      if (content) Fn(content, activeClass);
      // istanbul ignore else @preserve
      if (tabContent && nextContent && Hn(nextContent, fadeClass)) {
        const nextHeight = nextContent.scrollHeight;
        tabPrivate.set(element, {
          currentHeight,
          nextHeight,
          tab: null,
          content: null
        });
        Bn(tabContent, collapsingClass);
        tabContent.style.height = `${currentHeight}px`;
        no(tabContent);
        [content, nextContent].forEach((c) => {
          // istanbul ignore else @preserve
          if (c) Fn(c, "overflow-hidden");
        });
      }
      if (nextContent && nextContent && Hn(nextContent, fadeClass)) {
        setTimeout(() => {
          Bn(nextContent, showClass);
          qn(nextContent, () => {
            triggerTabShow(self);
          });
        }, 1);
      } else {
        // istanbul ignore else @preserve
        if (nextContent) Bn(nextContent, showClass);
        triggerTabShow(self);
      }
      // istanbul ignore else @preserve
      if (tab) K(tab, hiddenTabEvent);
    }
  };
  const getActiveTab = (self) => {
    const { nav } = self;
    // istanbul ignore next @preserve
    if (!l(nav)) {
      return { tab: null, content: null };
    }
    const activeTabs = Io(activeClass, nav);
    let tab = null;
    // istanbul ignore else @preserve
    if (activeTabs.length === 1 && !dropdownMenuClasses.some(
      (c) => Hn(activeTabs[0].parentElement, c)
    )) {
      [tab] = activeTabs;
    } else if (activeTabs.length > 1) {
      tab = activeTabs[activeTabs.length - 1];
    }
    const content = l(tab) ? getTargetElement(tab) : null;
    return { tab, content };
  };
  const getParentDropdown = (element) => {
    // istanbul ignore next @preserve
    if (!l(element)) return null;
    const dropdown = Ee(element, `.${dropdownMenuClasses.join(",.")}`);
    return dropdown ? Co(`.${dropdownMenuClasses[0]}-toggle`, dropdown) : null;
  };
  const tabClickHandler = (e2) => {
    const self = getTabInstance(e2.target);
    // istanbul ignore else @preserve
    if (self) {
      e2.preventDefault();
      self.show();
    }
  };
  class Tab extends BaseComponent {
    /** @param target the target element */
    constructor(target) {
      super(target);
      /**
       * Toggles on/off the `click` event listener.
       *
       * @param add when `true`, event listener is added
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        action(this.element, ft, tabClickHandler);
      });
      const { element } = this;
      const content = getTargetElement(element);
      // istanbul ignore else @preserve
      if (content) {
        const nav = Ee(element, ".nav");
        const container = Ee(content, ".tab-content");
        this.nav = nav;
        this.content = content;
        this.tabContent = container;
        this.dropdown = getParentDropdown(element);
        const { tab } = getActiveTab(this);
        if (nav && !tab) {
          const firstTab = Co(tabSelector, nav);
          const firstTabContent = firstTab && getTargetElement(firstTab);
          // istanbul ignore else @preserve
          if (firstTabContent) {
            Bn(firstTab, activeClass);
            Bn(firstTabContent, showClass);
            Bn(firstTabContent, activeClass);
            In(element, De, "true");
          }
        }
        this._toggleEventListeners(true);
      }
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
      // istanbul ignore else @preserve
      if (!(nav && so.get(nav)) && !Hn(element, activeClass)) {
        const { tab, content } = getActiveTab(this);
        // istanbul ignore else @preserve
        if (nav) {
          tabPrivate.set(nav, { tab, content, currentHeight: 0, nextHeight: 0 });
        }
        hideTabEvent.relatedTarget = element;
        // istanbul ignore else @preserve
        if (l(tab)) {
          K(tab, hideTabEvent);
          // istanbul ignore else @preserve
          if (!hideTabEvent.defaultPrevented) {
            Bn(element, activeClass);
            In(element, De, "true");
            const activeDropdown = l(tab) && getParentDropdown(tab);
            if (activeDropdown && Hn(activeDropdown, activeClass)) {
              Fn(activeDropdown, activeClass);
            }
            // istanbul ignore else @preserve
            if (nav) {
              const toggleTab = () => {
                // istanbul ignore else @preserve
                if (tab) {
                  Fn(tab, activeClass);
                  In(tab, De, "false");
                }
                if (dropdown && !Hn(dropdown, activeClass)) {
                  Bn(dropdown, activeClass);
                }
              };
              if (content && (Hn(content, fadeClass) || nextContent && Hn(nextContent, fadeClass))) {
                so.set(nav, toggleTab, 1);
              } else toggleTab();
            }
            // istanbul ignore else @preserve
            if (content) {
              Fn(content, showClass);
              if (Hn(content, fadeClass)) {
                qn(content, () => triggerTabHide(this));
              } else {
                triggerTabHide(this);
              }
            }
          }
        }
      }
    }
    /** Removes the `Tab` component from the target element. */
    dispose() {
      this._toggleEventListeners();
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
  const getToastInstance = (element) => Rn(element, toastComponent);
  const toastInitCallback = (element) => new Toast(element);
  const showToastEvent = to(
    `show.bs.${toastString}`
  );
  const shownToastEvent = to(
    `shown.bs.${toastString}`
  );
  const hideToastEvent = to(
    `hide.bs.${toastString}`
  );
  const hiddenToastEvent = to(
    `hidden.bs.${toastString}`
  );
  const showToastComplete = (self) => {
    const { element, options } = self;
    Fn(element, showingClass);
    so.clear(element, showingClass);
    K(element, shownToastEvent);
    // istanbul ignore else @preserve
    if (options.autohide) {
      so.set(element, () => self.hide(), options.delay, toastString);
    }
  };
  const hideToastComplete = (self) => {
    const { element } = self;
    Fn(element, showingClass);
    Fn(element, showClass);
    Bn(element, hideClass);
    so.clear(element, toastString);
    K(element, hiddenToastEvent);
  };
  const hideToast = (self) => {
    const { element, options } = self;
    Bn(element, showingClass);
    if (options.animation) {
      no(element);
      qn(element, () => hideToastComplete(self));
    } else {
      hideToastComplete(self);
    }
  };
  const showToast = (self) => {
    const { element, options } = self;
    so.set(
      element,
      () => {
        Fn(element, hideClass);
        no(element);
        Bn(element, showClass);
        Bn(element, showingClass);
        if (options.animation) {
          qn(element, () => showToastComplete(self));
        } else {
          showToastComplete(self);
        }
      },
      17,
      showingClass
    );
  };
  const toastClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && Ee(target, toastToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getToastInstance(element);
    // istanbul ignore else @preserve
    if (self) {
      // istanbul ignore else @preserve
      if (trigger && trigger.tagName === "A") e2.preventDefault();
      self.relatedTarget = trigger;
      self.show();
    }
  };
  const interactiveToastHandler = (e2) => {
    const element = e2.target;
    const self = getToastInstance(element);
    const { type, relatedTarget } = e2;
    // istanbul ignore else @preserve: a solid filter is required
    if (self && element !== relatedTarget && !element.contains(relatedTarget)) {
      if ([bt, st].includes(type)) {
        so.clear(element, toastString);
      } else {
        so.set(element, () => self.hide(), self.options.delay, toastString);
      }
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
        // istanbul ignore else @preserve
        if (element && !isShown) {
          K(element, showToastEvent);
          if (!showToastEvent.defaultPrevented) {
            showToast(this);
          }
        }
      });
      /** Hides the toast. */
      __publicField(this, "hide", () => {
        const { element, isShown } = this;
        // istanbul ignore else @preserve
        if (element && isShown) {
          K(element, hideToastEvent);
          if (!hideToastEvent.defaultPrevented) {
            hideToast(this);
          }
        }
      });
      /**
       * Toggles on/off the `click` event listener.
       *
       * @param add when `true`, it will add the listener
       */
      __publicField(this, "_toggleEventListeners", (add) => {
        const action = add ? E$1 : r;
        const { element, triggers, dismiss, options, hide } = this;
        // istanbul ignore else @preserve
        if (dismiss) {
          action(dismiss, ft, hide);
        }
        // istanbul ignore else @preserve
        if (options.autohide) {
          [st, ct, bt, ht].forEach(
            (e2) => action(element, e2, interactiveToastHandler)
          );
        }
        // istanbul ignore else @preserve
        if (triggers.length) {
          triggers.forEach(
            (btn) => action(btn, ft, toastClickHandler)
          );
        }
      });
      const { element, options } = this;
      if (options.animation && !Hn(element, fadeClass)) {
        Bn(element, fadeClass);
      } else if (!options.animation && Hn(element, fadeClass)) {
        Fn(element, fadeClass);
      }
      this.dismiss = Co(toastDismissSelector, element);
      this.triggers = [
        ...ue(toastToggleSelector, d(element))
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this._toggleEventListeners(true);
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
      return Hn(this.element, showClass);
    }
    /** Removes the `Toast` component from the target element. */
    dispose() {
      const { element, isShown } = this;
      this._toggleEventListeners();
      so.clear(element, toastString);
      if (isShown) {
        Fn(element, showClass);
      }
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
    [...collection].forEach((x2) => callback(x2));
  };
  const removeComponentDataAPI = (component, context) => {
    const compData = L.getAllFor(component);
    if (compData) {
      [...compData].forEach(([element, instance]) => {
        if (context.contains(element)) instance.dispose();
      });
    }
  };
  const initCallback = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    const elemCollection = [...be("*", lookUp)];
    $n(componentsList).forEach((cs) => {
      const { init, selector } = cs;
      initComponentDataAPI(
        init,
        elemCollection.filter((item) => xo(item, selector))
      );
    });
  };
  const removeDataAPI = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    Zn(componentsList).forEach((comp) => {
      removeComponentDataAPI(comp, lookUp);
    });
  };
  if (document.body) initCallback();
  else {
    E$1(document, "DOMContentLoaded", () => initCallback(), { once: true });
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
