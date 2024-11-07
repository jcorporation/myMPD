var BSN = function(exports) {
  "use strict";
  const De = "aria-describedby", Oe = "aria-expanded", $ = "aria-hidden", Pe = "aria-modal", Be = "aria-pressed", Fe = "aria-selected", rt = "focus", ct = "focusin", at = "focusout", dt = "keydown", pt = "keyup", mt = "click", bt = "mousedown", ht = "hover", yt = "mouseenter", wt = "mouseleave", Dt = "pointerdown", xt = "pointermove", zt = "pointerup", Rt = "touchstart", je = "dragstart", qt = 'a[href], button, input, textarea, select, details, [tabindex]:not([tabindex="-1"]', on = "ArrowDown", sn = "ArrowUp", rn = "ArrowLeft", cn = "ArrowRight", gn = "Escape", _t = "transitionDuration", $t = "transitionDelay", M = "transitionend", W = "transitionProperty", zn = () => {
    const t = /(iPhone|iPod|iPad)/;
    return navigator?.userAgentData?.brands.some(
      (e2) => t.test(e2.brand)
    ) || t.test(
      navigator?.userAgent
    ) || false;
  }, te = () => {
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
  }, j = (t, e2) => t.getAttribute(e2), ne = (t, e2) => t.hasAttribute(e2), Qn = (t, e2, n) => t.setAttribute(e2, n), Gn = (t, e2) => t.removeAttribute(e2), qn = (t, ...e2) => {
    t.classList.add(...e2);
  }, Zn = (t, ...e2) => {
    t.classList.remove(...e2);
  }, Yn = (t, e2) => t.classList.contains(e2), b$1 = (t) => t != null && typeof t == "object" || false, l = (t) => b$1(t) && typeof t.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some(
    (e2) => t.nodeType === e2
  ) || false, u = (t) => l(t) && t.nodeType === 1 || false, h = /* @__PURE__ */ new Map(), O = {
    data: h,
    /**
     * Sets web components data.
     *
     * @param element target element
     * @param component the component's name or a unique key
     * @param instance the component instance
     */
    set: (t, e2, n) => {
      if (!u(t)) return;
      h.has(e2) || h.set(e2, /* @__PURE__ */ new Map()), h.get(e2).set(t, n);
    },
    /**
     * Returns all instances for specified component.
     *
     * @param component the component's name or a unique key
     * @returns all the component instances
     */
    getAllFor: (t) => h.get(t) || null,
    /**
     * Returns the instance associated with the target.
     *
     * @param element target element
     * @param component the component's name or a unique key
     * @returns the instance
     */
    get: (t, e2) => {
      if (!u(t) || !e2) return null;
      const n = O.getAllFor(e2);
      return t && n && n.get(t) || null;
    },
    /**
     * Removes web components data.
     *
     * @param element target element
     * @param component the component's name or a unique key
     */
    remove: (t, e2) => {
      const n = O.getAllFor(e2);
      !n || !u(t) || (n.delete(t), n.size === 0 && h.delete(e2));
    }
  }, to = (t, e2) => O.get(t, e2), k$1 = (t) => typeof t == "string" || false, G = (t) => b$1(t) && t.constructor.name === "Window" || false, K = (t) => l(t) && t.nodeType === 9 || false, d = (t) => K(t) ? t : l(t) ? t.ownerDocument : G(t) ? t.document : globalThis.document, N = (t, ...e2) => Object.assign(t, ...e2), oe = (t) => {
    if (!t) return;
    if (k$1(t))
      return d().createElement(t);
    const { tagName: e2 } = t, n = oe(e2);
    if (!n) return;
    const o = { ...t };
    return delete o.tagName, N(n, o);
  }, q = (t, e2) => t.dispatchEvent(e2), g = (t, e2, n) => {
    const o = getComputedStyle(t, n), s = e2.replace("webkit", "Webkit").replace(/([A-Z])/g, "-$1").toLowerCase();
    return o.getPropertyValue(s);
  }, ae = (t) => {
    const e2 = g(t, W), n = g(t, $t), o = n.includes("ms") ? 1 : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : 0;
    return Number.isNaN(s) ? 0 : s;
  }, ie = (t) => {
    const e2 = g(t, W), n = g(t, _t), o = n.includes("ms") ? 1 : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : 0;
    return Number.isNaN(s) ? 0 : s;
  }, ro = (t, e2) => {
    let n = 0;
    const o = new Event(M), s = ie(t), r2 = ae(t);
    if (s) {
      const a = (i) => {
        i.target === t && (e2.apply(t, [i]), t.removeEventListener(M, a), n = 1);
      };
      t.addEventListener(M, a), setTimeout(() => {
        n || q(t, o);
      }, s + r2 + 17);
    } else
      e2.apply(t, [o]);
  }, io = (t, e2) => t.focus(e2), P = (t) => ["true", true].includes(t) ? true : ["false", false].includes(t) ? false : ["null", "", null, void 0].includes(t) ? null : t !== "" && !Number.isNaN(+t) ? +t : t, T = (t) => Object.entries(t), ue = (t) => t.toLowerCase(), lo = (t, e2, n, o) => {
    if (!u(t)) return e2;
    const s = { ...n }, r2 = { ...t.dataset }, a = { ...e2 }, i = {}, f2 = "title";
    return T(r2).forEach(([c, p2]) => {
      const E2 = typeof c == "string" && c.includes(o) ? c.replace(o, "").replace(
        /[A-Z]/g,
        (C) => ue(C)
      ) : c;
      i[E2] = P(p2);
    }), T(s).forEach(([c, p2]) => {
      s[c] = P(p2);
    }), T(e2).forEach(([c, p2]) => {
      c in s ? a[c] = s[c] : c in i ? a[c] = i[c] : a[c] = c === f2 ? j(t, f2) : p2;
    }), a;
  }, po = (t) => Object.keys(t), vo = (t, e2) => {
    const n = new CustomEvent(t, {
      cancelable: true,
      bubbles: true
    });
    return b$1(e2) && N(n, e2), n;
  }, bo = { passive: true }, Eo = (t) => t.offsetHeight, ho = (t, e2) => {
    T(e2).forEach(([n, o]) => {
      if (o && k$1(n) && n.includes("--"))
        t.style.setProperty(n, o);
      else {
        const s = {};
        s[n] = o, N(t.style, s);
      }
    });
  }, x = (t) => b$1(t) && t.constructor.name === "Map" || false, le = (t) => typeof t == "number" || false, v$1 = /* @__PURE__ */ new Map(), yo = {
    /**
     * Sets a new timeout timer for an element, or element -> key association.
     *
     * @param element target element
     * @param callback the callback
     * @param delay the execution delay
     * @param key a unique key
     */
    set: (t, e2, n, o) => {
      u(t) && (o && o.length ? (v$1.has(t) || v$1.set(t, /* @__PURE__ */ new Map()), v$1.get(t).set(o, setTimeout(e2, n))) : v$1.set(t, setTimeout(e2, n)));
    },
    /**
     * Returns the timer associated with the target.
     *
     * @param element target element
     * @param key a unique
     * @returns the timer
     */
    get: (t, e2) => {
      if (!u(t)) return null;
      const n = v$1.get(t);
      return e2 && n && x(n) ? n.get(e2) || null : le(n) ? n : null;
    },
    /**
     * Clears the element's timer.
     *
     * @param element target element
     * @param key a unique key
     */
    clear: (t, e2) => {
      if (!u(t)) return;
      const n = v$1.get(t);
      e2 && e2.length && x(n) ? (clearTimeout(n.get(e2)), n.delete(e2), n.size === 0 && v$1.delete(t)) : (clearTimeout(n), v$1.delete(t));
    }
  }, de = (t, e2) => (l(e2) ? e2 : d()).querySelectorAll(t), z = /* @__PURE__ */ new Map();
  function fe(t) {
    const { shiftKey: e2, code: n } = t, o = d(this), s = [
      ...de(qt, this)
    ].filter(
      (i) => !ne(i, "disabled") && !j(i, $)
    );
    if (!s.length) return;
    const r2 = s[0], a = s[s.length - 1];
    n === "Tab" && (e2 && o.activeElement === r2 ? (a.focus(), t.preventDefault()) : !e2 && o.activeElement === a && (r2.focus(), t.preventDefault()));
  }
  const pe = (t) => z.has(t) === true, Ao = (t) => {
    const e2 = pe(t);
    (e2 ? Q : R)(t, "keydown", fe), e2 ? z.delete(t) : z.set(t, true);
  }, m$1 = (t) => u(t) && "offsetWidth" in t || false, w = (t, e2) => {
    const { width: n, height: o, top: s, right: r2, bottom: a, left: i } = t.getBoundingClientRect();
    let f2 = 1, c = 1;
    if (e2 && m$1(t)) {
      const { offsetWidth: p2, offsetHeight: E2 } = t;
      f2 = p2 > 0 ? Math.round(n) / p2 : 1, c = E2 > 0 ? Math.round(o) / E2 : 1;
    }
    return {
      width: n / f2,
      height: o / c,
      top: s / c,
      right: r2 / f2,
      bottom: a / c,
      left: i / f2,
      x: i / f2,
      y: s / c
    };
  }, So = (t) => d(t).body, S = (t) => d(t).documentElement, ko = (t) => {
    const e2 = G(t), n = e2 ? t.scrollX : t.scrollLeft, o = e2 ? t.scrollY : t.scrollTop;
    return { x: n, y: o };
  }, me = (t) => l(t) && t.constructor.name === "ShadowRoot" || false, A = (t) => t.nodeName === "HTML" ? t : u(t) && t.assignedSlot || l(t) && t.parentNode || me(t) && t.host || S(t), ve = (t) => t ? K(t) ? t.defaultView : l(t) ? t?.ownerDocument?.defaultView : t : window, be = (t) => l(t) && ["TABLE", "TD", "TH"].includes(t.nodeName) || false, Ee = (t, e2) => t.matches(e2), we = (t) => {
    if (!m$1(t)) return false;
    const { width: e2, height: n } = w(t), { offsetWidth: o, offsetHeight: s } = t;
    return Math.round(e2) !== o || Math.round(n) !== s;
  }, Co = (t, e2, n) => {
    const o = m$1(e2), s = w(
      t,
      o && we(e2)
    ), r2 = { x: 0, y: 0 };
    if (o) {
      const a = w(e2, true);
      r2.x = a.x + e2.clientLeft, r2.y = a.y + e2.clientTop;
    }
    return {
      x: s.left + n.x - r2.x,
      y: s.top + n.y - r2.y,
      width: s.width,
      height: s.height
    };
  };
  let F = 0, H = 0;
  const y$1 = /* @__PURE__ */ new Map(), Ae = (t, e2) => {
    let n = e2 ? F : H;
    if (e2) {
      const o = Ae(t), s = y$1.get(o) || /* @__PURE__ */ new Map();
      y$1.has(o) || y$1.set(o, s), x(s) && !s.has(e2) ? (s.set(e2, n), F += 1) : n = s.get(e2);
    } else {
      const o = t.id || t;
      y$1.has(o) ? n = y$1.get(o) : (y$1.set(o, n), H += 1);
    }
    return n;
  }, Se = (t) => Array.isArray(t) || false, Do = (t) => {
    if (!l(t)) return false;
    const { top: e2, bottom: n } = w(t), { clientHeight: o } = S(t);
    return e2 <= o && n >= 0;
  }, zo = (t) => typeof t == "function" || false, Vo = (t) => b$1(t) && t.constructor.name === "NodeList" || false, Uo = (t) => S(t).dir === "rtl", ke = (t, e2) => !t || !e2 ? null : t.closest(e2) || ke(t.getRootNode().host, e2) || null, Ro = (t, e2) => u(t) ? t : (u(e2) ? e2 : d()).querySelector(t), Ne = (t, e2) => (l(e2) ? e2 : d()).getElementsByTagName(
    t
  ), Go = (t, e2) => (e2 && l(e2) ? e2 : d()).getElementsByClassName(
    t
  );
  const e = {}, f = (t) => {
    const { type: n, currentTarget: c } = t;
    e[n].forEach((a, s) => {
      c === s && a.forEach((o, i) => {
        i.apply(s, [t]), typeof o == "object" && o.once && r(s, n, i, o);
      });
    });
  }, E$1 = (t, n, c, a) => {
    e[n] || (e[n] = /* @__PURE__ */ new Map());
    const s = e[n];
    s.has(t) || s.set(t, /* @__PURE__ */ new Map());
    const o = s.get(
      t
    ), { size: i } = o;
    o.set(c, a), i || t.addEventListener(
      n,
      f,
      a
    );
  }, r = (t, n, c, a) => {
    const s = e[n], o = s && s.get(t), i = o && o.get(c), d2 = i !== void 0 ? i : a;
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
  const version = "5.1.0";
  const Version = version;
  class BaseComponent {
    /**
     * @param target `Element` or selector string
     * @param config component instance options
     */
    constructor(target, config) {
      let element;
      try {
        if (u(target)) {
          element = target;
        } else if (k$1(target)) {
          element = Ro(target);
          if (!element) throw Error(`"${target}" is not a valid selector.`);
        } else {
          throw Error(`your target is not an instance of HTMLElement.`);
        }
      } catch (e2) {
        throw Error(`${this.name} Error: ${e2.message}`);
      }
      const prevInstance = O.get(element, this.name);
      if (prevInstance) {
        prevInstance._toggleEventListeners();
      }
      this.element = element;
      this.options = this.defaults && po(this.defaults).length ? lo(element, this.defaults, config || {}, "bs") : {};
      O.set(element, this.name, this);
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
      O.remove(this.element, this.name);
      po(this).forEach((prop) => {
        delete this[prop];
      });
    }
  }
  const alertSelector = `.${alertString}`;
  const alertDismissSelector = `[${dataBsDismiss}="${alertString}"]`;
  const getAlertInstance = (element) => to(element, alertComponent);
  const alertInitCallback = (element) => new Alert(element);
  const closeAlertEvent = vo(
    `close.bs.${alertString}`
  );
  const closedAlertEvent = vo(
    `closed.bs.${alertString}`
  );
  const alertTransitionEnd = (self) => {
    const { element } = self;
    q(element, closedAlertEvent);
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
      this.dismiss = Ro(
        alertDismissSelector,
        this.element
      );
      this._toggleEventListeners(true);
    }
    get name() {
      return alertComponent;
    }
    /**
     * Public method that hides the `.alert` element from the user,
     * disposes the instance once animation is complete, then
     * removes the element from the DOM.
     */
    close = () => {
      const { element } = this;
      if (element && Yn(element, showClass)) {
        q(element, closeAlertEvent);
        if (!closeAlertEvent.defaultPrevented) {
          Zn(element, showClass);
          if (Yn(element, fadeClass)) {
            ro(element, () => alertTransitionEnd(this));
          } else alertTransitionEnd(this);
        }
      }
    };
    /**
     * Toggle on / off the `click` event listener.
     *
     * @param add when `true`, event listener is added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      const { dismiss, close } = this;
      if (dismiss) action(dismiss, mt, close);
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
  const getButtonInstance = (element) => to(element, buttonComponent);
  const buttonInitCallback = (element) => new Button(element);
  class Button extends BaseComponent {
    static selector = buttonSelector;
    static init = buttonInitCallback;
    static getInstance = getButtonInstance;
    /**
     * @param target usually a `.btn` element
     */
    constructor(target) {
      super(target);
      const { element } = this;
      this.isActive = Yn(element, activeClass);
      Qn(element, Be, String(!!this.isActive));
      this._toggleEventListeners(true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return buttonComponent;
    }
    /**
     * Toggles the state of the target button.
     *
     * @param e usually `click` Event object
     */
    toggle = (e2) => {
      if (e2) e2.preventDefault();
      const { element, isActive } = this;
      if (!Yn(element, "disabled") && !j(element, "disabled")) {
        const action = isActive ? Zn : qn;
        action(element, activeClass);
        Qn(element, Be, isActive ? "false" : "true");
        this.isActive = Yn(element, activeClass);
      }
    };
    /**
     * Toggles on/off the `click` event listener.
     *
     * @param add when `true`, event listener is added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      action(this.element, mt, this.toggle);
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
        return att === dataBsParent ? ke(element, attValue) : Ro(attValue, doc);
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
  const getCarouselInstance = (element) => to(element, carouselComponent);
  const carouselInitCallback = (element) => new Carousel(element);
  let startX = 0;
  let currentX = 0;
  let endX = 0;
  const carouselSlideEvent = vo(`slide.bs.${carouselString}`);
  const carouselSlidEvent = vo(`slid.bs.${carouselString}`);
  const carouselTransitionEndHandler = (self) => {
    const { index, direction, element, slides, options } = self;
    if (self.isAnimating) {
      const activeItem = getActiveIndex(self);
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      qn(slides[index], activeClass);
      Zn(slides[index], `${carouselItem}-${orientation}`);
      Zn(slides[index], `${carouselItem}-${directionClass}`);
      Zn(slides[activeItem], activeClass);
      Zn(slides[activeItem], `${carouselItem}-${directionClass}`);
      q(element, carouselSlidEvent);
      yo.clear(element, dataBsSlide);
      if (self.cycle && !d(element).hidden && options.interval && !self.isPaused) {
        self.cycle();
      }
    }
  };
  function carouselPauseHandler() {
    const self = getCarouselInstance(this);
    if (self && !self.isPaused && !yo.get(this, pausedClass)) {
      qn(this, pausedClass);
    }
  }
  function carouselResumeHandler() {
    const self = getCarouselInstance(this);
    if (self && self.isPaused && !yo.get(this, pausedClass)) {
      self.cycle();
    }
  }
  function carouselIndicatorHandler(e2) {
    e2.preventDefault();
    const element = ke(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    if (self && !self.isAnimating) {
      const newIndex = +(j(this, dataBsSlideTo) || 0);
      if (this && !Yn(this, activeClass) && !Number.isNaN(newIndex)) {
        self.to(newIndex);
      }
    }
  }
  function carouselControlsHandler(e2) {
    e2.preventDefault();
    const element = ke(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    if (self && !self.isAnimating) {
      const orientation = j(this, dataBsSlide);
      if (orientation === "next") {
        self.next();
      } else if (orientation === "prev") {
        self.prev();
      }
    }
  }
  const carouselKeyHandler = ({ code, target }) => {
    const doc = d(target);
    const [element] = [...de(carouselSelector, doc)].filter((x2) => Do(x2));
    const self = getCarouselInstance(element);
    if (self && !self.isAnimating && !/textarea|input/i.test(target.nodeName)) {
      const RTL = Uo(element);
      const arrowKeyNext = !RTL ? cn : rn;
      const arrowKeyPrev = !RTL ? rn : cn;
      if (code === arrowKeyPrev) self.prev();
      else if (code === arrowKeyNext) self.next();
    }
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
    if (self && !self.isAnimating && !self.isTouch) {
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
  }
  const carouselPointerMoveHandler = (e2) => {
    currentX = e2.pageX;
  };
  const carouselPointerUpHandler = (e2) => {
    const { target } = e2;
    const doc = d(target);
    const self = [...de(carouselSelector, doc)].map((c) => getCarouselInstance(c)).find((i) => i.isTouch);
    if (self) {
      const { element, index } = self;
      const RTL = Uo(element);
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
    }
  };
  const activateCarouselIndicator = (self, index) => {
    const { indicators } = self;
    [...indicators].forEach((x2) => Zn(x2, activeClass));
    if (self.indicators[index]) qn(indicators[index], activeClass);
  };
  const toggleCarouselTouchHandlers = (self, add) => {
    const { element } = self;
    const action = add ? E$1 : r;
    action(
      d(element),
      xt,
      carouselPointerMoveHandler,
      bo
    );
    action(
      d(element),
      zt,
      carouselPointerUpHandler,
      bo
    );
  };
  const getActiveIndex = (self) => {
    const { slides, element } = self;
    const activeItem = Ro(`.${carouselItem}.${activeClass}`, element);
    return m$1(activeItem) ? [...slides].indexOf(activeItem) : -1;
  };
  class Carousel extends BaseComponent {
    static selector = carouselSelector;
    static init = carouselInitCallback;
    static getInstance = getCarouselInstance;
    /**
     * @param target mostly a `.carousel` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.direction = Uo(element) ? "right" : "left";
      this.isTouch = false;
      this.slides = Go(carouselItem, element);
      const { slides } = this;
      if (slides.length >= 2) {
        const activeIndex = getActiveIndex(this);
        const transitionItem = [...slides].find(
          (s) => Ee(s, `.${carouselItem}-next,.${carouselItem}-next`)
        );
        this.index = activeIndex;
        const doc = d(element);
        this.controls = [
          ...de(`[${dataBsSlide}]`, element),
          ...de(
            `[${dataBsSlide}][${dataBsTarget}="#${element.id}"]`,
            doc
          )
        ].filter((c, i, ar) => i === ar.indexOf(c));
        this.indicator = Ro(
          `.${carouselString}-indicators`,
          element
        );
        this.indicators = [
          ...this.indicator ? de(`[${dataBsSlideTo}]`, this.indicator) : [],
          ...de(
            `[${dataBsSlideTo}][${dataBsTarget}="#${element.id}"]`,
            doc
          )
        ].filter((c, i, ar) => i === ar.indexOf(c));
        const { options } = this;
        this.options.interval = options.interval === true ? carouselDefaults.interval : options.interval;
        if (transitionItem) {
          this.index = [...slides].indexOf(transitionItem);
        } else if (activeIndex < 0) {
          this.index = 0;
          qn(slides[0], activeClass);
          if (this.indicators.length) activateCarouselIndicator(this, 0);
        }
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
      return Yn(this.element, pausedClass);
    }
    /**
     * Check if instance is animating.
     */
    get isAnimating() {
      return Ro(
        `.${carouselItem}-next,.${carouselItem}-prev`,
        this.element
      ) !== null;
    }
    cycle() {
      const { element, options, isPaused, index } = this;
      yo.clear(element, carouselString);
      if (isPaused) {
        yo.clear(element, pausedClass);
        Zn(element, pausedClass);
      }
      yo.set(
        element,
        () => {
          if (this.element && !this.isPaused && !this.isTouch && Do(element)) {
            this.to(index + 1);
          }
        },
        options.interval,
        carouselString
      );
    }
    pause() {
      const { element, options } = this;
      if (!this.isPaused && options.interval) {
        qn(element, pausedClass);
        yo.set(
          element,
          () => {
          },
          1,
          pausedClass
        );
      }
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
    /**
     * Jump to the item with the `idx` index.
     *
     * @param idx the index of the item to jump to
     */
    to(idx) {
      const { element, slides, options } = this;
      const activeItem = getActiveIndex(this);
      const RTL = Uo(element);
      let next = idx;
      if (!this.isAnimating && activeItem !== next && !yo.get(element, dataBsSlide)) {
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
        N(carouselSlideEvent, eventProperties);
        N(carouselSlidEvent, eventProperties);
        q(element, carouselSlideEvent);
        if (!carouselSlideEvent.defaultPrevented) {
          this.index = next;
          activateCarouselIndicator(this, next);
          if (ie(slides[next]) && Yn(element, "slide")) {
            yo.set(
              element,
              () => {
                qn(slides[next], `${carouselItem}-${orientation}`);
                Eo(slides[next]);
                qn(slides[next], `${carouselItem}-${directionClass}`);
                qn(slides[activeItem], `${carouselItem}-${directionClass}`);
                ro(
                  slides[next],
                  () => this.slides && this.slides.length && carouselTransitionEndHandler(this)
                );
              },
              0,
              dataBsSlide
            );
          } else {
            qn(slides[next], activeClass);
            Zn(slides[activeItem], activeClass);
            yo.set(
              element,
              () => {
                yo.clear(element, dataBsSlide);
                if (element && options.interval && !this.isPaused) {
                  this.cycle();
                }
                q(element, carouselSlidEvent);
              },
              0,
              dataBsSlide
            );
          }
        }
      }
    }
    /**
     * Toggles all event listeners for the `Carousel` instance.
     *
     * @param add when `TRUE` event listeners are added
     */
    _toggleEventListeners = (add) => {
      const { element, options, slides, controls, indicators } = this;
      const { touch, pause, interval, keyboard } = options;
      const action = add ? E$1 : r;
      if (pause && interval) {
        action(element, yt, carouselPauseHandler);
        action(element, wt, carouselResumeHandler);
      }
      if (touch && slides.length > 2) {
        action(
          element,
          Dt,
          carouselPointerDownHandler,
          bo
        );
        action(element, Rt, carouselDragHandler, { passive: false });
        action(element, je, carouselDragHandler, { passive: false });
      }
      if (controls.length) {
        controls.forEach((arrow) => {
          if (arrow) action(arrow, mt, carouselControlsHandler);
        });
      }
      if (indicators.length) {
        indicators.forEach((indicator) => {
          action(indicator, mt, carouselIndicatorHandler);
        });
      }
      if (keyboard) {
        action(d(element), dt, carouselKeyHandler);
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
        ro(clone.slides[clone.index], () => {
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
  const getCollapseInstance = (element) => to(element, collapseComponent);
  const collapseInitCallback = (element) => new Collapse(element);
  const showCollapseEvent = vo(`show.bs.${collapseString}`);
  const shownCollapseEvent = vo(`shown.bs.${collapseString}`);
  const hideCollapseEvent = vo(`hide.bs.${collapseString}`);
  const hiddenCollapseEvent = vo(`hidden.bs.${collapseString}`);
  const expandCollapse = (self) => {
    const { element, parent, triggers } = self;
    q(element, showCollapseEvent);
    if (!showCollapseEvent.defaultPrevented) {
      yo.set(element, te, 17);
      if (parent) yo.set(parent, te, 17);
      qn(element, collapsingClass);
      Zn(element, collapseString);
      ho(element, { height: `${element.scrollHeight}px` });
      ro(element, () => {
        yo.clear(element);
        if (parent) yo.clear(parent);
        triggers.forEach((btn) => Qn(btn, Oe, "true"));
        Zn(element, collapsingClass);
        qn(element, collapseString);
        qn(element, showClass);
        ho(element, { height: "" });
        q(element, shownCollapseEvent);
      });
    }
  };
  const collapseContent = (self) => {
    const { element, parent, triggers } = self;
    q(element, hideCollapseEvent);
    if (!hideCollapseEvent.defaultPrevented) {
      yo.set(element, te, 17);
      if (parent) yo.set(parent, te, 17);
      ho(element, { height: `${element.scrollHeight}px` });
      Zn(element, collapseString);
      Zn(element, showClass);
      qn(element, collapsingClass);
      Eo(element);
      ho(element, { height: "0px" });
      ro(element, () => {
        yo.clear(element);
        if (parent) yo.clear(parent);
        triggers.forEach((btn) => Qn(btn, Oe, "false"));
        Zn(element, collapsingClass);
        qn(element, collapseString);
        ho(element, { height: "" });
        q(element, hiddenCollapseEvent);
      });
    }
  };
  const collapseClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && ke(target, collapseToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getCollapseInstance(element);
    if (self) self.toggle();
    if (trigger && trigger.tagName === "A") e2.preventDefault();
  };
  class Collapse extends BaseComponent {
    static selector = collapseSelector;
    static init = collapseInitCallback;
    static getInstance = getCollapseInstance;
    /**
     * @param target and `Element` that matches the selector
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      const doc = d(element);
      this.triggers = [...de(collapseToggleSelector, doc)].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.parent = m$1(options.parent) ? options.parent : k$1(options.parent) ? getTargetElement(element) || Ro(options.parent, doc) : null;
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
    hide() {
      const { triggers, element } = this;
      if (!yo.get(element)) {
        collapseContent(this);
        if (triggers.length) {
          triggers.forEach((btn) => qn(btn, `${collapseString}d`));
        }
      }
    }
    show() {
      const { element, parent, triggers } = this;
      let activeCollapse;
      let activeCollapseInstance;
      if (parent) {
        activeCollapse = [
          ...de(`.${collapseString}.${showClass}`, parent)
        ].find((i) => getCollapseInstance(i));
        activeCollapseInstance = activeCollapse && getCollapseInstance(activeCollapse);
      }
      if ((!parent || !yo.get(parent)) && !yo.get(element)) {
        if (activeCollapseInstance && activeCollapse !== element) {
          collapseContent(activeCollapseInstance);
          activeCollapseInstance.triggers.forEach((btn) => {
            qn(btn, `${collapseString}d`);
          });
        }
        expandCollapse(this);
        if (triggers.length) {
          triggers.forEach((btn) => Zn(btn, `${collapseString}d`));
        }
      }
    }
    toggle() {
      if (!Yn(this.element, showClass)) this.show();
      else this.hide();
    }
    /**
     * Toggles on/off the event listener(s) of the `Collapse` instance.
     *
     * @param add when `true`, the event listener is added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      const { triggers } = this;
      if (triggers.length) {
        triggers.forEach(
          (btn) => action(btn, mt, collapseClickHandler)
        );
      }
    };
    dispose() {
      this._toggleEventListeners();
      super.dispose();
    }
  }
  const dropdownMenuClasses = ["dropdown", "dropup", "dropstart", "dropend"];
  const dropdownComponent = "Dropdown";
  const dropdownMenuClass = "dropdown-menu";
  const isEmptyAnchor = (element) => {
    const parentAnchor = ke(element, "A");
    return element.tagName === "A" && ne(element, "href") && j(element, "href").slice(-1) === "#" || parentAnchor && ne(parentAnchor, "href") && j(parentAnchor, "href").slice(-1) === "#";
  };
  const [dropdownString, dropupString, dropstartString, dropendString] = dropdownMenuClasses;
  const dropdownSelector = `[${dataBsToggle}="${dropdownString}"]`;
  const getDropdownInstance = (element) => to(element, dropdownComponent);
  const dropdownInitCallback = (element) => new Dropdown(element);
  const dropdownMenuEndClass = `${dropdownMenuClass}-end`;
  const verticalClass = [dropdownString, dropupString];
  const horizontalClass = [dropstartString, dropendString];
  const menuFocusTags = ["A", "BUTTON"];
  const dropdownDefaults = {
    offset: 5,
    display: "dynamic"
  };
  const showDropdownEvent = vo(
    `show.bs.${dropdownString}`
  );
  const shownDropdownEvent = vo(
    `shown.bs.${dropdownString}`
  );
  const hideDropdownEvent = vo(
    `hide.bs.${dropdownString}`
  );
  const hiddenDropdownEvent = vo(`hidden.bs.${dropdownString}`);
  const updatedDropdownEvent = vo(`updated.bs.${dropdownString}`);
  const styleDropdown = (self) => {
    const { element, menu, parentElement, options } = self;
    const { offset } = options;
    if (g(menu, "position") !== "static") {
      const RTL = Uo(element);
      const menuEnd = Yn(menu, dropdownMenuEndClass);
      const resetProps = ["margin", "top", "bottom", "left", "right"];
      resetProps.forEach((p2) => {
        const style = {};
        style[p2] = "";
        ho(menu, style);
      });
      let positionClass = dropdownMenuClasses.find(
        (c) => Yn(parentElement, c)
      ) || dropdownString;
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
      const { clientWidth, clientHeight } = S(element);
      const {
        left: targetLeft,
        top: targetTop,
        width: targetWidth,
        height: targetHeight
      } = w(element);
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
        N(dropdownPosition[positionClass], {
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
          N(dropdownPosition[positionClass], posAjust);
        }
      }
      const margins = dropdownMargin[positionClass];
      ho(menu, {
        ...dropdownPosition[positionClass],
        margin: `${margins.map((x2) => x2 ? `${x2}px` : x2).join(" ")}`
      });
      if (verticalClass.includes(positionClass) && menuEnd) {
        if (menuEnd) {
          const endAdjust = !RTL && leftExceed || RTL && rightExceed ? "menuStart" : "menuEnd";
          ho(menu, dropdownPosition[endAdjust]);
        }
      }
      q(parentElement, updatedDropdownEvent);
    }
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
    const action = self.open ? E$1 : r;
    const doc = d(element);
    action(doc, mt, dropdownDismissHandler);
    action(doc, rt, dropdownDismissHandler);
    action(doc, dt, dropdownPreventScroll);
    action(doc, pt, dropdownKeyHandler);
    if (options.display === "dynamic") {
      if (self.open) self._observer.observe(menu);
      else self._observer.disconnect();
    }
  };
  const getCurrentOpenDropdown = (element) => {
    const currentParent = [...dropdownMenuClasses, "btn-group", "input-group"].map(
      (c) => Go(`${c} ${showClass}`, d(element))
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
    if (!m$1(target)) return;
    const element = getCurrentOpenDropdown(target);
    const self = element && getDropdownInstance(element);
    if (!self) return;
    const { parentElement, menu } = self;
    const isForm = parentElement && parentElement.contains(target) && (target.tagName === "form" || ke(target, "form") !== null);
    if ([mt, bt].includes(type) && isEmptyAnchor(target)) {
      e2.preventDefault();
    }
    if (!isForm && type !== rt && target !== element && target !== menu) {
      self.hide();
    }
  };
  const dropdownClickHandler = (e2) => {
    const { target } = e2;
    const element = target && ke(target, dropdownSelector);
    const self = element && getDropdownInstance(element);
    if (!self) return;
    e2.stopPropagation();
    self.toggle();
    if (element && isEmptyAnchor(element)) e2.preventDefault();
  };
  const dropdownPreventScroll = (e2) => {
    if ([on, sn].includes(e2.code)) e2.preventDefault();
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
    if (menuItems && menuItems.length && [on, sn].includes(code)) {
      let idx = menuItems.indexOf(activeElement);
      if (activeElement === element) {
        idx = 0;
      } else if (code === sn) {
        idx = idx > 1 ? idx - 1 : 0;
      } else if (code === on) {
        idx = idx < menuItems.length - 1 ? idx + 1 : idx;
      }
      if (menuItems[idx]) io(menuItems[idx]);
    }
    if (gn === code && open) {
      self.toggle();
      io(element);
    }
  }
  function dropdownIntersectionHandler(target) {
    const element = getCurrentOpenDropdown(target);
    const self = element && getDropdownInstance(element);
    if (self && self.open) styleDropdown(self);
  }
  class Dropdown extends BaseComponent {
    static selector = dropdownSelector;
    static init = dropdownInitCallback;
    static getInstance = getDropdownInstance;
    /**
     * @param target Element or string selector
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { parentElement } = this.element;
      const [menu] = Go(
        dropdownMenuClass,
        parentElement
      );
      if (!menu) return;
      this.parentElement = parentElement;
      this.menu = menu;
      this._observer = new IntersectionObserver(
        ([entry]) => dropdownIntersectionHandler(entry.target),
        { threshold: 1 }
      );
      this._toggleEventListeners(true);
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
      q(parentElement, showDropdownEvent);
      if (showDropdownEvent.defaultPrevented) return;
      qn(menu, showClass);
      qn(parentElement, showClass);
      Qn(element, Oe, "true");
      styleDropdown(this);
      this.open = !open;
      io(element);
      toggleDropdownDismiss(this);
      q(parentElement, shownDropdownEvent);
    }
    hide() {
      const { element, open, menu, parentElement } = this;
      if (!open) return;
      [hideDropdownEvent, hiddenDropdownEvent].forEach((e2) => {
        e2.relatedTarget = element;
      });
      q(parentElement, hideDropdownEvent);
      if (hideDropdownEvent.defaultPrevented) return;
      Zn(menu, showClass);
      Zn(parentElement, showClass);
      Qn(element, Oe, "false");
      this.open = !open;
      toggleDropdownDismiss(this);
      q(parentElement, hiddenDropdownEvent);
    }
    /**
     * Toggles on/off the `click` event listener of the `Dropdown`.
     *
     * @param add when `true`, it will add the event listener
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      action(this.element, mt, dropdownClickHandler);
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
    ...Go(fixedTopClass, parent),
    ...Go(fixedBottomClass, parent),
    ...Go(stickyTopClass, parent),
    ...Go(positionStickyClass, parent),
    ...Go("is-fixed", parent)
  ];
  const resetScrollbar = (element) => {
    const bd = So(element);
    ho(bd, {
      paddingRight: "",
      overflow: ""
    });
    const fixedItems = getFixedItems(bd);
    if (fixedItems.length) {
      fixedItems.forEach((fixed) => {
        ho(fixed, {
          paddingRight: "",
          marginRight: ""
        });
      });
    }
  };
  const measureScrollbar = (element) => {
    const { clientWidth } = S(element);
    const { innerWidth } = ve(element);
    return Math.abs(innerWidth - clientWidth);
  };
  const setScrollbar = (element, overflow) => {
    const bd = So(element);
    const bodyPad = parseInt(g(bd, "paddingRight"), 10);
    const isOpen = g(bd, "overflow") === "hidden";
    const sbWidth = isOpen && bodyPad ? 0 : measureScrollbar(element);
    const fixedItems = getFixedItems(bd);
    if (!overflow) return;
    ho(bd, {
      overflow: "hidden",
      paddingRight: `${bodyPad + sbWidth}px`
    });
    if (!fixedItems.length) return;
    fixedItems.forEach((fixed) => {
      const itemPadValue = g(fixed, "paddingRight");
      fixed.style.paddingRight = `${parseInt(itemPadValue, 10) + sbWidth}px`;
      if ([stickyTopClass, positionStickyClass].some((c) => Yn(fixed, c))) {
        const itemMValue = g(fixed, "marginRight");
        fixed.style.marginRight = `${parseInt(itemMValue, 10) - sbWidth}px`;
      }
    });
  };
  const offcanvasString = "offcanvas";
  const popupContainer = oe({
    tagName: "div",
    className: "popup-container"
  });
  const appendPopup = (target, customContainer) => {
    const containerIsBody = l(customContainer) && customContainer.nodeName === "BODY";
    const lookup = l(customContainer) && !containerIsBody ? customContainer : popupContainer;
    const BODY = containerIsBody ? customContainer : So(target);
    if (l(target)) {
      if (lookup === popupContainer) {
        BODY.append(popupContainer);
      }
      lookup.append(target);
    }
  };
  const removePopup = (target, customContainer) => {
    const containerIsBody = l(customContainer) && customContainer.nodeName === "BODY";
    const lookup = l(customContainer) && !containerIsBody ? customContainer : popupContainer;
    if (l(target)) {
      target.remove();
      if (lookup === popupContainer && !popupContainer.children.length) {
        popupContainer.remove();
      }
    }
  };
  const hasPopup = (target, customContainer) => {
    const lookup = l(customContainer) && customContainer.nodeName !== "BODY" ? customContainer : popupContainer;
    return l(target) && lookup.contains(target);
  };
  const backdropString = "backdrop";
  const modalBackdropClass = `${modalString}-${backdropString}`;
  const offcanvasBackdropClass = `${offcanvasString}-${backdropString}`;
  const modalActiveSelector = `.${modalString}.${showClass}`;
  const offcanvasActiveSelector = `.${offcanvasString}.${showClass}`;
  const overlay = oe("div");
  const getCurrentOpen = (element) => {
    return Ro(
      `${modalActiveSelector},${offcanvasActiveSelector}`,
      d(element)
    );
  };
  const toggleOverlayType = (isModal) => {
    const targetClass = isModal ? modalBackdropClass : offcanvasBackdropClass;
    [modalBackdropClass, offcanvasBackdropClass].forEach((c) => {
      Zn(overlay, c);
    });
    qn(overlay, targetClass);
  };
  const appendOverlay = (element, hasFade, isModal) => {
    toggleOverlayType(isModal);
    appendPopup(overlay, So(element));
    if (hasFade) qn(overlay, fadeClass);
  };
  const showOverlay = () => {
    if (!Yn(overlay, showClass)) {
      qn(overlay, showClass);
      Eo(overlay);
    }
  };
  const hideOverlay = () => {
    Zn(overlay, showClass);
  };
  const removeOverlay = (element) => {
    if (!getCurrentOpen(element)) {
      Zn(overlay, fadeClass);
      removePopup(overlay, So(element));
      resetScrollbar(element);
    }
  };
  const isVisible = (element) => {
    return m$1(element) && g(element, "visibility") !== "hidden" && element.offsetParent !== null;
  };
  const modalSelector = `.${modalString}`;
  const modalToggleSelector = `[${dataBsToggle}="${modalString}"]`;
  const modalDismissSelector = `[${dataBsDismiss}="${modalString}"]`;
  const modalStaticClass = `${modalString}-static`;
  const modalDefaults = {
    backdrop: true,
    keyboard: true
  };
  const getModalInstance = (element) => to(element, modalComponent);
  const modalInitCallback = (element) => new Modal(element);
  const showModalEvent = vo(
    `show.bs.${modalString}`
  );
  const shownModalEvent = vo(
    `shown.bs.${modalString}`
  );
  const hideModalEvent = vo(
    `hide.bs.${modalString}`
  );
  const hiddenModalEvent = vo(
    `hidden.bs.${modalString}`
  );
  const setModalScrollbar = (self) => {
    const { element } = self;
    const scrollbarWidth = measureScrollbar(element);
    const { clientHeight, scrollHeight } = S(element);
    const { clientHeight: modalHeight, scrollHeight: modalScrollHeight } = element;
    const modalOverflow = modalHeight !== modalScrollHeight;
    if (!modalOverflow && scrollbarWidth) {
      const pad = !Uo(element) ? "paddingRight" : "paddingLeft";
      const padStyle = { [pad]: `${scrollbarWidth}px` };
      ho(element, padStyle);
    }
    setScrollbar(element, modalOverflow || clientHeight !== scrollHeight);
  };
  const toggleModalDismiss = (self, add) => {
    const action = add ? E$1 : r;
    const { element } = self;
    action(element, mt, modalDismissHandler);
    action(d(element), dt, modalKeyHandler);
    if (add) self._observer.observe(element);
    else self._observer.disconnect();
  };
  const afterModalHide = (self) => {
    const { triggers, element, relatedTarget } = self;
    removeOverlay(element);
    ho(element, { paddingRight: "", display: "" });
    toggleModalDismiss(self);
    const focusElement = showModalEvent.relatedTarget || triggers.find(isVisible);
    if (focusElement) io(focusElement);
    hiddenModalEvent.relatedTarget = relatedTarget || void 0;
    q(element, hiddenModalEvent);
    Ao(element);
  };
  const afterModalShow = (self) => {
    const { element, relatedTarget } = self;
    io(element);
    toggleModalDismiss(self, true);
    shownModalEvent.relatedTarget = relatedTarget || void 0;
    q(element, shownModalEvent);
    Ao(element);
  };
  const beforeModalShow = (self) => {
    const { element, hasFade } = self;
    ho(element, { display: "block" });
    setModalScrollbar(self);
    if (!getCurrentOpen(element)) {
      ho(So(element), { overflow: "hidden" });
    }
    qn(element, showClass);
    Gn(element, $);
    Qn(element, Pe, "true");
    if (hasFade) ro(element, () => afterModalShow(self));
    else afterModalShow(self);
  };
  const beforeModalHide = (self) => {
    const { element, options, hasFade } = self;
    if (options.backdrop && hasFade && Yn(overlay, showClass) && !getCurrentOpen(element)) {
      hideOverlay();
      ro(overlay, () => afterModalHide(self));
    } else {
      afterModalHide(self);
    }
  };
  const modalClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && ke(target, modalToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getModalInstance(element);
    if (!self) return;
    if (trigger && trigger.tagName === "A") e2.preventDefault();
    self.relatedTarget = trigger;
    self.toggle();
  };
  const modalKeyHandler = ({ code, target }) => {
    const element = Ro(modalActiveSelector, d(target));
    const self = element && getModalInstance(element);
    if (!self) return;
    const { options } = self;
    if (options.keyboard && code === gn && Yn(element, showClass)) {
      self.relatedTarget = null;
      self.hide();
    }
  };
  const modalDismissHandler = (e2) => {
    const { currentTarget } = e2;
    const self = currentTarget && getModalInstance(currentTarget);
    if (!self || !currentTarget || yo.get(currentTarget)) return;
    const { options, isStatic, modalDialog } = self;
    const { backdrop } = options;
    const { target } = e2;
    const selectedText = d(currentTarget)?.getSelection()?.toString().length;
    const targetInsideDialog = modalDialog.contains(target);
    const dismiss = target && ke(target, modalDismissSelector);
    if (isStatic && !targetInsideDialog) {
      yo.set(
        currentTarget,
        () => {
          qn(currentTarget, modalStaticClass);
          ro(modalDialog, () => staticTransitionEnd(self));
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
    const duration = (ie(modalDialog) || 0) + 17;
    Zn(element, modalStaticClass);
    yo.set(element, () => yo.clear(element), duration);
  };
  class Modal extends BaseComponent {
    static selector = modalSelector;
    static init = modalInitCallback;
    static getInstance = getModalInstance;
    /**
     * @param target usually the `.modal` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      const modalDialog = Ro(
        `.${modalString}-dialog`,
        element
      );
      if (!modalDialog) return;
      this.modalDialog = modalDialog;
      this.triggers = [
        ...de(
          modalToggleSelector,
          d(element)
        )
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.isStatic = this.options.backdrop === "static";
      this.hasFade = Yn(element, fadeClass);
      this.relatedTarget = null;
      this._observer = new ResizeObserver(() => this.update());
      this._toggleEventListeners(true);
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
    toggle() {
      if (Yn(this.element, showClass)) this.hide();
      else this.show();
    }
    show() {
      const { element, options, hasFade, relatedTarget } = this;
      const { backdrop } = options;
      let overlayDelay = 0;
      if (Yn(element, showClass)) return;
      showModalEvent.relatedTarget = relatedTarget || void 0;
      q(element, showModalEvent);
      if (showModalEvent.defaultPrevented) return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getModalInstance(currentOpen) || to(
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
        overlayDelay = ie(overlay);
        showOverlay();
        setTimeout(() => beforeModalShow(this), overlayDelay);
      } else {
        beforeModalShow(this);
        if (currentOpen && Yn(overlay, showClass)) {
          hideOverlay();
        }
      }
    }
    hide() {
      const { element, hasFade, relatedTarget } = this;
      if (!Yn(element, showClass)) return;
      hideModalEvent.relatedTarget = relatedTarget || void 0;
      q(element, hideModalEvent);
      if (hideModalEvent.defaultPrevented) return;
      Zn(element, showClass);
      Qn(element, $, "true");
      Gn(element, Pe);
      if (hasFade) ro(element, () => beforeModalHide(this));
      else beforeModalHide(this);
    }
    /**
     * Updates the modal layout.
     */
    update = () => {
      if (Yn(this.element, showClass)) setModalScrollbar(this);
    };
    /**
     * Toggles on/off the `click` event listener of the `Modal` instance.
     *
     * @param add when `true`, event listener(s) is/are added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      const { triggers } = this;
      if (!triggers.length) return;
      triggers.forEach((btn) => action(btn, mt, modalClickHandler));
    };
    dispose() {
      const clone = { ...this };
      const { modalDialog, hasFade } = clone;
      const callback = () => setTimeout(() => super.dispose(), 17);
      this.hide();
      this._toggleEventListeners();
      if (hasFade) {
        ro(modalDialog, callback);
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
  const getOffcanvasInstance = (element) => to(element, offcanvasComponent);
  const offcanvasInitCallback = (element) => new Offcanvas(element);
  const showOffcanvasEvent = vo(`show.bs.${offcanvasString}`);
  const shownOffcanvasEvent = vo(`shown.bs.${offcanvasString}`);
  const hideOffcanvasEvent = vo(`hide.bs.${offcanvasString}`);
  const hiddenOffcanvasEvent = vo(`hidden.bs.${offcanvasString}`);
  const setOffCanvasScrollbar = (self) => {
    const { element } = self;
    const { clientHeight, scrollHeight } = S(element);
    setScrollbar(element, clientHeight !== scrollHeight);
  };
  const toggleOffCanvasDismiss = (self, add) => {
    const action = add ? E$1 : r;
    const doc = d(self.element);
    action(doc, dt, offcanvasKeyDismissHandler);
    action(doc, mt, offcanvasDismissHandler);
  };
  const beforeOffcanvasShow = (self) => {
    const { element, options } = self;
    if (!options.scroll) {
      setOffCanvasScrollbar(self);
      ho(So(element), { overflow: "hidden" });
    }
    qn(element, offcanvasTogglingClass);
    qn(element, showClass);
    ho(element, { visibility: "visible" });
    ro(element, () => showOffcanvasComplete(self));
  };
  const beforeOffcanvasHide = (self) => {
    const { element, options } = self;
    const currentOpen = getCurrentOpen(element);
    element.blur();
    if (!currentOpen && options.backdrop && Yn(overlay, showClass)) {
      hideOverlay();
    }
    ro(element, () => hideOffcanvasComplete(self));
  };
  const offcanvasTriggerHandler = (e2) => {
    const trigger = ke(e2.target, offcanvasToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getOffcanvasInstance(element);
    if (!self) return;
    self.relatedTarget = trigger;
    self.toggle();
    if (trigger?.tagName === "A") e2.preventDefault();
  };
  const offcanvasDismissHandler = (e2) => {
    const { target } = e2;
    const element = Ro(
      offcanvasActiveSelector,
      d(target)
    );
    if (!element) return;
    const offCanvasDismiss = Ro(
      offcanvasDismissSelector,
      element
    );
    const self = getOffcanvasInstance(element);
    if (!self) return;
    const { options, triggers } = self;
    const { backdrop } = options;
    const trigger = ke(target, offcanvasToggleSelector);
    const selection = d(element).getSelection();
    if (overlay.contains(target) && backdrop === "static") return;
    if (!(selection && selection.toString().length) && (!element.contains(target) && backdrop && (!trigger || triggers.includes(target)) || offCanvasDismiss && offCanvasDismiss.contains(target))) {
      self.relatedTarget = offCanvasDismiss && offCanvasDismiss.contains(target) ? offCanvasDismiss : void 0;
      self.hide();
    }
    if (trigger && trigger.tagName === "A") e2.preventDefault();
  };
  const offcanvasKeyDismissHandler = ({ code, target }) => {
    const element = Ro(
      offcanvasActiveSelector,
      d(target)
    );
    const self = element && getOffcanvasInstance(element);
    if (!self) return;
    if (self.options.keyboard && code === gn) {
      self.relatedTarget = void 0;
      self.hide();
    }
  };
  const showOffcanvasComplete = (self) => {
    const { element } = self;
    Zn(element, offcanvasTogglingClass);
    Gn(element, $);
    Qn(element, Pe, "true");
    Qn(element, "role", "dialog");
    q(element, shownOffcanvasEvent);
    toggleOffCanvasDismiss(self, true);
    io(element);
    Ao(element);
  };
  const hideOffcanvasComplete = (self) => {
    const { element, triggers } = self;
    Qn(element, $, "true");
    Gn(element, Pe);
    Gn(element, "role");
    ho(element, { visibility: "" });
    const visibleTrigger = showOffcanvasEvent.relatedTarget || triggers.find(isVisible);
    if (visibleTrigger) io(visibleTrigger);
    removeOverlay(element);
    q(element, hiddenOffcanvasEvent);
    Zn(element, offcanvasTogglingClass);
    Ao(element);
    if (!getCurrentOpen(element)) {
      toggleOffCanvasDismiss(self);
    }
  };
  class Offcanvas extends BaseComponent {
    static selector = offcanvasSelector;
    static init = offcanvasInitCallback;
    static getInstance = getOffcanvasInstance;
    /**
     * @param target usually an `.offcanvas` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.triggers = [
        ...de(
          offcanvasToggleSelector,
          d(element)
        )
      ].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.relatedTarget = void 0;
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
    toggle() {
      if (Yn(this.element, showClass)) this.hide();
      else this.show();
    }
    show() {
      const { element, options, relatedTarget } = this;
      let overlayDelay = 0;
      if (Yn(element, showClass)) return;
      showOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      shownOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      q(element, showOffcanvasEvent);
      if (showOffcanvasEvent.defaultPrevented) return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getOffcanvasInstance(currentOpen) || to(
          currentOpen,
          modalComponent
        );
        if (that) that.hide();
      }
      if (options.backdrop) {
        if (!hasPopup(overlay)) appendOverlay(element, true);
        else toggleOverlayType();
        overlayDelay = ie(overlay);
        showOverlay();
        setTimeout(() => beforeOffcanvasShow(this), overlayDelay);
      } else {
        beforeOffcanvasShow(this);
        if (currentOpen && Yn(overlay, showClass)) hideOverlay();
      }
    }
    hide() {
      const { element, relatedTarget } = this;
      if (!Yn(element, showClass)) return;
      hideOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      hiddenOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      q(element, hideOffcanvasEvent);
      if (hideOffcanvasEvent.defaultPrevented) return;
      qn(element, offcanvasTogglingClass);
      Zn(element, showClass);
      beforeOffcanvasHide(this);
    }
    /**
     * Toggles on/off the `click` event listeners.
     *
     * @param self the `Offcanvas` instance
     * @param add when *true*, listeners are added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      this.triggers.forEach(
        (btn) => action(btn, mt, offcanvasTriggerHandler)
      );
    };
    dispose() {
      const { element } = this;
      const isOpen = Yn(element, showClass);
      const callback = () => setTimeout(() => super.dispose(), 1);
      this.hide();
      this._toggleEventListeners();
      if (isOpen) ro(element, callback);
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
      const RTL = Uo(element);
      const { x: scrollLeft, y: scrollTop } = ko(offsetParent);
      ho(tooltip, {
        top: "",
        left: "",
        right: "",
        bottom: ""
      });
      const { offsetWidth: tipWidth, offsetHeight: tipHeight } = tooltip;
      const { clientWidth: htmlcw, clientHeight: htmlch, offsetWidth: htmlow } = S(element);
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
      const observerEntry = self._observer.getEntry(element);
      const {
        width: elemWidth,
        height: elemHeight,
        left: elemRectLeft,
        right: elemRectRight,
        top: elemRectTop
      } = observerEntry?.boundingClientRect || w(element, true);
      const {
        x: elemOffsetLeft,
        y: elemOffsetTop
      } = Co(
        element,
        offsetParent,
        { x: scrollLeft, y: scrollTop }
      );
      ho(arrow, {
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
      ho(tooltip, {
        top: `${topPosition}px`,
        bottom: bottomPosition === "" ? "" : `${bottomPosition}px`,
        left: leftPosition === "auto" ? leftPosition : `${leftPosition}px`,
        right: rightPosition !== "" ? `${rightPosition}px` : ""
      });
      if (m$1(arrow)) {
        if (arrowTop !== "") {
          arrow.style.top = `${arrowTop}px`;
        }
        if (arrowLeft !== "") {
          arrow.style.left = `${arrowLeft}px`;
        } else if (arrowRight !== "") {
          arrow.style.right = `${arrowRight}px`;
        }
      }
      const updatedTooltipEvent = vo(
        `updated.bs.${ue(self.name)}`
      );
      q(element, updatedTooltipEvent);
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
  const p = (e2) => e2 != null && typeof e2 == "object" || false, k = (e2) => p(e2) && typeof e2.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some(
    (t) => e2.nodeType === t
  ) || false, b = (e2) => k(e2) && e2.nodeType === 1 || false, v = (e2) => typeof e2 == "function" || false, y = "1.0.2", m = "PositionObserver Error";
  class E {
    entries;
    static version = y;
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
    constructor(t, i) {
      if (!v(t))
        throw new Error(`${m}: ${t} is not a function.`);
      this.entries = /* @__PURE__ */ new Map(), this._callback = t, this._root = b(i?.root) ? i.root : document?.documentElement, this._tick = 0;
    }
    /**
     * Start observing the position of the specified element.
     * If the element is not currently attached to the DOM,
     * it will NOT be added to the entries.
     *
     * @param target an `Element` target
     */
    observe = (t) => {
      if (!b(t))
        throw new Error(
          `${m}: ${t} is not an instance of Element.`
        );
      this._root.contains(t) && this._new(t).then((i) => {
        this.getEntry(t) || this.entries.set(t, i), this._tick || (this._tick = requestAnimationFrame(this._runCallback));
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
      const t = new Promise((i) => {
        const c = [];
        this.entries.forEach(
          ({ target: s, boundingClientRect: n }) => {
            this._root.contains(s) && this._new(s).then(({ boundingClientRect: o, isVisible: h2 }) => {
              const { left: a, top: f2, bottom: _, right: l2 } = o;
              if (n.top !== f2 || n.left !== a || n.right !== l2 || n.bottom !== _) {
                const r2 = { target: s, boundingClientRect: o, isVisible: h2 };
                this.entries.set(s, r2), c.push(r2);
              }
            });
          }
        ), i(c);
      });
      this._tick = requestAnimationFrame(async () => {
        const i = await t;
        i.length && this._callback(i, this), this._runCallback();
      });
    };
    /**
     * Calculate the target bounding box and determine
     * the value of `isVisible`.
     *
     * @param target an `Element` target
     */
    _new = (t) => {
      const { clientWidth: i, clientHeight: c } = this._root;
      return new Promise((s) => {
        new IntersectionObserver(
          ([{ boundingClientRect: o }], h2) => {
            h2.disconnect();
            const { left: a, top: f2, bottom: _, right: l2, width: r2, height: u2 } = o, w2 = f2 > 1 - u2 && a > 1 - r2 && _ <= c + u2 - 1 && l2 <= i + r2 - 1;
            s({
              target: t,
              isVisible: w2,
              boundingClientRect: o
            });
          }
        ).observe(t);
      });
    };
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
  const dataOriginalTitle = "data-original-title";
  const tooltipComponent = "Tooltip";
  const setHtml = (element, content, sanitizeFn) => {
    if (k$1(content) && content.length) {
      let dirty = content.trim();
      if (zo(sanitizeFn)) dirty = sanitizeFn(dirty);
      const domParser = new DOMParser();
      const tempDocument = domParser.parseFromString(dirty, "text/html");
      element.append(...[...tempDocument.body.childNodes]);
    } else if (m$1(content)) {
      element.append(content);
    } else if (Vo(content) || Se(content) && content.every(l)) {
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
    if (Uo(element)) {
      tipPositions.left = "end";
      tipPositions.right = "start";
    }
    const placementClass = `bs-${tipString}-${tipPositions[placement]}`;
    let tooltipTemplate;
    if (m$1(template)) {
      tooltipTemplate = template;
    } else {
      const htmlMarkup = oe("div");
      setHtml(htmlMarkup, template, sanitizeFn);
      tooltipTemplate = htmlMarkup.firstChild;
    }
    if (!m$1(tooltipTemplate)) return;
    self.tooltip = tooltipTemplate.cloneNode(true);
    const { tooltip } = self;
    Qn(tooltip, "id", id);
    Qn(tooltip, "role", tooltipString);
    const bodyClass = isTooltip ? `${tooltipString}-inner` : `${popoverString}-body`;
    const tooltipHeader = isTooltip ? null : Ro(`.${popoverString}-header`, tooltip);
    const tooltipBody = Ro(`.${bodyClass}`, tooltip);
    self.arrow = Ro(
      `.${tipString}-arrow`,
      tooltip
    );
    const { arrow } = self;
    if (m$1(title)) titleParts = [title.cloneNode(true)];
    else {
      const tempTitle = oe("div");
      setHtml(tempTitle, title, sanitizeFn);
      titleParts = [...[...tempTitle.childNodes]];
    }
    if (m$1(content)) contentParts = [content.cloneNode(true)];
    else {
      const tempContent = oe("div");
      setHtml(tempContent, content, sanitizeFn);
      contentParts = [...[...tempContent.childNodes]];
    }
    if (dismissible) {
      if (title) {
        if (m$1(btnClose)) {
          titleParts = [...titleParts, btnClose.cloneNode(true)];
        } else {
          const tempBtn = oe("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          titleParts = [...titleParts, tempBtn.firstChild];
        }
      } else {
        if (tooltipHeader) tooltipHeader.remove();
        if (m$1(btnClose)) {
          contentParts = [...contentParts, btnClose.cloneNode(true)];
        } else {
          const tempBtn = oe("div");
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
      self.btn = Ro(".btn-close", tooltip) || void 0;
    } else if (title && tooltipBody) setHtml(tooltipBody, title, sanitizeFn);
    qn(tooltip, "position-absolute");
    qn(arrow, "position-absolute");
    if (!Yn(tooltip, tipString)) qn(tooltip, tipString);
    if (animation && !Yn(tooltip, fadeClass)) {
      qn(tooltip, fadeClass);
    }
    if (customClass && !Yn(tooltip, customClass)) {
      qn(tooltip, customClass);
    }
    if (!Yn(tooltip, placementClass)) qn(tooltip, placementClass);
  };
  const getElementContainer = (element) => {
    const majorBlockTags = ["HTML", "BODY"];
    const containers = [];
    let { parentNode } = element;
    while (parentNode && !majorBlockTags.includes(parentNode.nodeName)) {
      parentNode = A(parentNode);
      if (!(me(parentNode) || be(parentNode))) {
        containers.push(parentNode);
      }
    }
    return containers.find((c, i) => {
      if ((g(c, "position") !== "relative" || g(c, "position") === "relative" && c.offsetHeight !== c.scrollHeight) && containers.slice(i + 1).every(
        (r2) => g(r2, "position") === "static"
      )) {
        return c;
      }
      return null;
    }) || d(element).body;
  };
  const tooltipSelector = `[${dataBsToggle}="${tooltipString}"],[data-tip="${tooltipString}"]`;
  const titleAttr = "title";
  let getTooltipInstance = (element) => to(element, tooltipComponent);
  const tooltipInitCallback = (element) => new Tooltip(element);
  const removeTooltip = (self) => {
    const { element, tooltip, container } = self;
    Gn(element, De);
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
    if (ne(element, dataOriginalTitle) && self.name === tooltipComponent) {
      toggleTooltipTitle(self);
    }
    if (callback) callback();
  };
  const toggleTooltipAction = (self, add) => {
    const action = add ? E$1 : r;
    const { element } = self;
    action(
      d(element),
      Rt,
      self.handleTouch,
      bo
    );
  };
  const tooltipShownAction = (self) => {
    const { element } = self;
    const shownTooltipEvent = vo(
      `shown.bs.${ue(self.name)}`
    );
    toggleTooltipAction(self, true);
    q(element, shownTooltipEvent);
    yo.clear(element, "in");
  };
  const tooltipHiddenAction = (self) => {
    const { element } = self;
    const hiddenTooltipEvent = vo(
      `hidden.bs.${ue(self.name)}`
    );
    toggleTooltipAction(self);
    removeTooltip(self);
    q(element, hiddenTooltipEvent);
    yo.clear(element, "out");
  };
  const toggleTooltipOpenHandlers = (self, add) => {
    const action = add ? E$1 : r;
    const { element, tooltip } = self;
    const parentModal = ke(element, `.${modalString}`);
    const parentOffcanvas = ke(element, `.${offcanvasString}`);
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
    Qn(
      element,
      titleAtt[content ? 0 : 1],
      content || j(element, titleAtt[0]) || ""
    );
    Gn(element, titleAtt[content ? 1 : 0]);
  };
  class Tooltip extends BaseComponent {
    static selector = tooltipSelector;
    static init = tooltipInitCallback;
    static getInstance = getTooltipInstance;
    static styleTip = styleTip;
    /**
     * @param target the target element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      const isTooltip = this.name === tooltipComponent;
      const tipString = isTooltip ? tooltipString : popoverString;
      const tipComponent = isTooltip ? tooltipComponent : popoverComponent;
      getTooltipInstance = (elem) => to(elem, tipComponent);
      this.enabled = true;
      this.id = `${tipString}-${Ae(element, tipString)}`;
      const { options } = this;
      if (!options.title && isTooltip || !isTooltip && !options.content) {
        return;
      }
      N(tooltipDefaults, { titleAttr: "" });
      if (ne(element, titleAttr) && isTooltip && typeof options.title === "string") {
        toggleTooltipTitle(this, options.title);
      }
      const container = getElementContainer(element);
      const offsetParent = ["sticky", "fixed", "relative"].some(
        (position) => g(container, "position") === position
      ) ? container : ve(element);
      this.container = container;
      this.offsetParent = offsetParent;
      createTip(this);
      if (!this.tooltip) return;
      this._observer = new E((entries) => {
        if (!entries.some((entry) => entry.isVisible)) return;
        this.update();
      });
      this._toggleEventListeners(true);
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
    handleFocus = () => io(this.element);
    handleShow = () => this.show();
    show() {
      const { options, tooltip, element, container, id } = this;
      const { animation } = options;
      const outTimer = yo.get(element, "out");
      yo.clear(element, "out");
      if (tooltip && !outTimer && !hasTip(this)) {
        yo.set(
          element,
          () => {
            const showTooltipEvent = vo(
              `show.bs.${ue(this.name)}`
            );
            q(element, showTooltipEvent);
            if (!showTooltipEvent.defaultPrevented) {
              appendPopup(tooltip, container);
              Qn(element, De, `#${id}`);
              this.update();
              toggleTooltipOpenHandlers(this, true);
              if (!Yn(tooltip, showClass)) qn(tooltip, showClass);
              if (animation) {
                ro(tooltip, () => tooltipShownAction(this));
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
      yo.clear(element, "in");
      if (tooltip && hasTip(this)) {
        yo.set(
          element,
          () => {
            const hideTooltipEvent = vo(
              `hide.bs.${ue(this.name)}`
            );
            q(element, hideTooltipEvent);
            if (!hideTooltipEvent.defaultPrevented) {
              this.update();
              Zn(tooltip, showClass);
              toggleTooltipOpenHandlers(this);
              if (animation) {
                ro(tooltip, () => tooltipHiddenAction(this));
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
    /**
     * Handles the `touchstart` event listener for `Tooltip`
     *
     * @this {Tooltip}
     * @param {TouchEvent} e the `Event` object
     */
    handleTouch = ({ target }) => {
      const { tooltip, element } = this;
      if (tooltip && tooltip.contains(target) || target === element || target && element.contains(target)) ;
      else {
        this.hide();
      }
    };
    /**
     * Toggles on/off the `Tooltip` event listeners.
     *
     * @param add when `true`, event listeners are added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      const { element, options, btn } = this;
      const { trigger } = options;
      const isPopover = this.name !== tooltipComponent;
      const dismissible = isPopover && options.dismissible ? true : false;
      if (!trigger.includes("manual")) {
        this.enabled = !!add;
        const triggerOptions = trigger.split(" ");
        triggerOptions.forEach((tr) => {
          if (tr === ht) {
            action(element, bt, this.handleShow);
            action(element, yt, this.handleShow);
            if (!dismissible) {
              action(element, wt, this.handleHide);
              action(
                d(element),
                Rt,
                this.handleTouch,
                bo
              );
            }
          } else if (tr === mt) {
            action(element, tr, !dismissible ? this.toggle : this.handleShow);
          } else if (tr === rt) {
            action(element, ct, this.handleShow);
            if (!dismissible) action(element, at, this.handleHide);
            if (zn()) {
              action(element, mt, this.handleFocus);
            }
          }
          if (dismissible && btn) {
            action(btn, mt, this.handleHide);
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
        ro(tooltip, callback);
      } else {
        callback();
      }
    }
  }
  const popoverSelector = `[${dataBsToggle}="${popoverString}"],[data-tip="${popoverString}"]`;
  const popoverDefaults = N({}, tooltipDefaults, {
    template: getTipTemplate(popoverString),
    content: "",
    dismissible: false,
    btnClose: '<button class="btn-close" aria-label="Close"></button>'
  });
  const getPopoverInstance = (element) => to(element, popoverComponent);
  const popoverInitCallback = (element) => new Popover(element);
  class Popover extends Tooltip {
    static selector = popoverSelector;
    static init = popoverInitCallback;
    static getInstance = getPopoverInstance;
    static styleTip = styleTip;
    /**
     * @param target the target element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
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
    show = () => {
      super.show();
      const { options, btn } = this;
      if (options.dismissible && btn) setTimeout(() => io(btn), 17);
    };
  }
  const tabString = "tab";
  const tabComponent = "Tab";
  const tabSelector = `[${dataBsToggle}="${tabString}"]`;
  const getTabInstance = (element) => to(element, tabComponent);
  const tabInitCallback = (element) => new Tab(element);
  const showTabEvent = vo(
    `show.bs.${tabString}`
  );
  const shownTabEvent = vo(
    `shown.bs.${tabString}`
  );
  const hideTabEvent = vo(
    `hide.bs.${tabString}`
  );
  const hiddenTabEvent = vo(
    `hidden.bs.${tabString}`
  );
  const tabPrivate = /* @__PURE__ */ new Map();
  const triggerTabEnd = (self) => {
    const { tabContent, nav } = self;
    if (tabContent && Yn(tabContent, collapsingClass)) {
      tabContent.style.height = "";
      Zn(tabContent, collapsingClass);
    }
    if (nav) yo.clear(nav);
  };
  const triggerTabShow = (self) => {
    const { element, tabContent, content: nextContent, nav } = self;
    const { tab } = m$1(nav) && tabPrivate.get(nav) || { tab: null };
    if (tabContent && nextContent && Yn(nextContent, fadeClass)) {
      const { currentHeight, nextHeight } = tabPrivate.get(element) || { currentHeight: 0, nextHeight: 0 };
      if (currentHeight !== nextHeight) {
        setTimeout(() => {
          tabContent.style.height = `${nextHeight}px`;
          Eo(tabContent);
          ro(tabContent, () => triggerTabEnd(self));
        }, 50);
      } else {
        triggerTabEnd(self);
      }
    } else if (nav) yo.clear(nav);
    shownTabEvent.relatedTarget = tab;
    q(element, shownTabEvent);
  };
  const triggerTabHide = (self) => {
    const { element, content: nextContent, tabContent, nav } = self;
    const { tab, content } = nav && tabPrivate.get(nav) || { tab: null, content: null };
    let currentHeight = 0;
    if (tabContent && nextContent && Yn(nextContent, fadeClass)) {
      [content, nextContent].forEach((c) => {
        if (c) qn(c, "overflow-hidden");
      });
      currentHeight = content ? content.scrollHeight : 0;
    }
    showTabEvent.relatedTarget = tab;
    hiddenTabEvent.relatedTarget = element;
    q(element, showTabEvent);
    if (showTabEvent.defaultPrevented) return;
    if (nextContent) qn(nextContent, activeClass);
    if (content) Zn(content, activeClass);
    if (tabContent && nextContent && Yn(nextContent, fadeClass)) {
      const nextHeight = nextContent.scrollHeight;
      tabPrivate.set(element, {
        currentHeight,
        nextHeight,
        tab: null,
        content: null
      });
      qn(tabContent, collapsingClass);
      tabContent.style.height = `${currentHeight}px`;
      Eo(tabContent);
      [content, nextContent].forEach((c) => {
        if (c) Zn(c, "overflow-hidden");
      });
    }
    if (nextContent && nextContent && Yn(nextContent, fadeClass)) {
      setTimeout(() => {
        qn(nextContent, showClass);
        ro(nextContent, () => {
          triggerTabShow(self);
        });
      }, 1);
    } else {
      if (nextContent) qn(nextContent, showClass);
      triggerTabShow(self);
    }
    if (tab) q(tab, hiddenTabEvent);
  };
  const getActiveTab = (self) => {
    const { nav } = self;
    if (!m$1(nav)) {
      return { tab: null, content: null };
    }
    const activeTabs = Go(
      activeClass,
      nav
    );
    let tab = null;
    if (activeTabs.length === 1 && !dropdownMenuClasses.some(
      (c) => Yn(activeTabs[0].parentElement, c)
    )) {
      [tab] = activeTabs;
    } else if (activeTabs.length > 1) {
      tab = activeTabs[activeTabs.length - 1];
    }
    const content = m$1(tab) ? getTargetElement(tab) : null;
    return { tab, content };
  };
  const getParentDropdown = (element) => {
    if (!m$1(element)) return null;
    const dropdown = ke(element, `.${dropdownMenuClasses.join(",.")}`);
    return dropdown ? Ro(`.${dropdownMenuClasses[0]}-toggle`, dropdown) : null;
  };
  const tabClickHandler = (e2) => {
    const self = getTabInstance(e2.target);
    e2.preventDefault();
    if (self) self.show();
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
      const nav = ke(element, ".nav");
      const container = ke(
        content,
        ".tab-content"
      );
      this.nav = nav;
      this.content = content;
      this.tabContent = container;
      this.dropdown = getParentDropdown(element);
      const { tab } = getActiveTab(this);
      if (nav && !tab) {
        const firstTab = Ro(tabSelector, nav);
        const firstTabContent = firstTab && getTargetElement(firstTab);
        if (firstTabContent) {
          qn(firstTab, activeClass);
          qn(firstTabContent, showClass);
          qn(firstTabContent, activeClass);
          Qn(element, Fe, "true");
        }
      }
      this._toggleEventListeners(true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return tabComponent;
    }
    show() {
      const { element, content: nextContent, nav, dropdown } = this;
      if (nav && yo.get(nav) || Yn(element, activeClass)) return;
      const { tab, content } = getActiveTab(this);
      if (nav && tab) {
        tabPrivate.set(nav, { tab, content, currentHeight: 0, nextHeight: 0 });
      }
      hideTabEvent.relatedTarget = element;
      if (!m$1(tab)) return;
      q(tab, hideTabEvent);
      if (hideTabEvent.defaultPrevented) return;
      qn(element, activeClass);
      Qn(element, Fe, "true");
      const activeDropdown = m$1(tab) && getParentDropdown(tab);
      if (activeDropdown && Yn(activeDropdown, activeClass)) {
        Zn(activeDropdown, activeClass);
      }
      if (nav) {
        const toggleTab = () => {
          if (tab) {
            Zn(tab, activeClass);
            Qn(tab, Fe, "false");
          }
          if (dropdown && !Yn(dropdown, activeClass)) {
            qn(dropdown, activeClass);
          }
        };
        if (content && (Yn(content, fadeClass) || nextContent && Yn(nextContent, fadeClass))) {
          yo.set(nav, toggleTab, 1);
        } else toggleTab();
      }
      if (content) {
        Zn(content, showClass);
        if (Yn(content, fadeClass)) {
          ro(content, () => triggerTabHide(this));
        } else {
          triggerTabHide(this);
        }
      }
    }
    /**
     * Toggles on/off the `click` event listener.
     *
     * @param add when `true`, event listener is added
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      action(this.element, mt, tabClickHandler);
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
  const getToastInstance = (element) => to(element, toastComponent);
  const toastInitCallback = (element) => new Toast(element);
  const showToastEvent = vo(
    `show.bs.${toastString}`
  );
  const shownToastEvent = vo(
    `shown.bs.${toastString}`
  );
  const hideToastEvent = vo(
    `hide.bs.${toastString}`
  );
  const hiddenToastEvent = vo(
    `hidden.bs.${toastString}`
  );
  const showToastComplete = (self) => {
    const { element, options } = self;
    Zn(element, showingClass);
    yo.clear(element, showingClass);
    q(element, shownToastEvent);
    if (options.autohide) {
      yo.set(element, () => self.hide(), options.delay, toastString);
    }
  };
  const hideToastComplete = (self) => {
    const { element } = self;
    Zn(element, showingClass);
    Zn(element, showClass);
    qn(element, hideClass);
    yo.clear(element, toastString);
    q(element, hiddenToastEvent);
  };
  const hideToast = (self) => {
    const { element, options } = self;
    qn(element, showingClass);
    if (options.animation) {
      Eo(element);
      ro(element, () => hideToastComplete(self));
    } else {
      hideToastComplete(self);
    }
  };
  const showToast = (self) => {
    const { element, options } = self;
    yo.set(
      element,
      () => {
        Zn(element, hideClass);
        Eo(element);
        qn(element, showClass);
        qn(element, showingClass);
        if (options.animation) {
          ro(element, () => showToastComplete(self));
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
    const trigger = target && ke(target, toastToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getToastInstance(element);
    if (!self) return;
    if (trigger && trigger.tagName === "A") e2.preventDefault();
    self.relatedTarget = trigger;
    self.show();
  };
  const interactiveToastHandler = (e2) => {
    const element = e2.target;
    const self = getToastInstance(element);
    const { type, relatedTarget } = e2;
    if (!self || element === relatedTarget || element.contains(relatedTarget)) return;
    if ([yt, ct].includes(type)) {
      yo.clear(element, toastString);
    } else {
      yo.set(element, () => self.hide(), self.options.delay, toastString);
    }
  };
  class Toast extends BaseComponent {
    static selector = toastSelector;
    static init = toastInitCallback;
    static getInstance = getToastInstance;
    /**
     * @param target the target `.toast` element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      if (options.animation && !Yn(element, fadeClass)) {
        qn(element, fadeClass);
      } else if (!options.animation && Yn(element, fadeClass)) {
        Zn(element, fadeClass);
      }
      this.dismiss = Ro(toastDismissSelector, element);
      this.triggers = [
        ...de(
          toastToggleSelector,
          d(element)
        )
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
      return Yn(this.element, showClass);
    }
    show = () => {
      const { element, isShown } = this;
      if (!element || isShown) return;
      q(element, showToastEvent);
      if (!showToastEvent.defaultPrevented) showToast(this);
    };
    hide = () => {
      const { element, isShown } = this;
      if (!element || !isShown) return;
      q(element, hideToastEvent);
      if (!hideToastEvent.defaultPrevented) hideToast(this);
    };
    /**
     * Toggles on/off the `click` event listener.
     *
     * @param add when `true`, it will add the listener
     */
    _toggleEventListeners = (add) => {
      const action = add ? E$1 : r;
      const { element, triggers, dismiss, options, hide } = this;
      if (dismiss) {
        action(dismiss, mt, hide);
      }
      if (options.autohide) {
        [ct, at, yt, wt].forEach(
          (e2) => action(element, e2, interactiveToastHandler)
        );
      }
      if (triggers.length) {
        triggers.forEach(
          (btn) => action(btn, mt, toastClickHandler)
        );
      }
    };
    dispose() {
      const { element, isShown } = this;
      this._toggleEventListeners();
      yo.clear(element, toastString);
      if (isShown) Zn(element, showClass);
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
    const compData = O.getAllFor(component);
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
    const elemCollection = [...Ne("*", lookUp)];
    componentsList.forEach((cs) => {
      const { init, selector } = cs;
      initComponentDataAPI(
        init,
        elemCollection.filter((item) => Ee(item, selector))
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
    E$1(document, "DOMContentLoaded", () => initCallback(), {
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
