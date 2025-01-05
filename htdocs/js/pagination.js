"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module pagination_js */

/**
 * Go's to the previous or next page
 * @param {string} direction on of next, prev
 * @param {number} limit maximum entries to display
 * @returns {void}
 */
function gotoPageDir(direction, limit) {
    let offset = app.current.offset;
    switch(direction) {
        case 'next':
            offset = offset + app.current.limit;
            break;
        case 'prev':
            offset = offset - app.current.limit;
            if (offset < 0) {
                offset = 0;
            }
            break;
        default:
            logError('Invalid goto page direction: ' + direction);
    }
    gotoPage(offset, limit);
}

/**
 * Go's to the page defined by offset
 * @param {number} offset page offset
 * @param {number} limit maximum entries to display
 * @returns {void}
 */
function gotoPage(offset, limit) {
    app.current.offset = offset;
    if (limit !== undefined) {
        if (limit === 0) {
            limit = maxElementsPerPage;
        }
        app.current.limit = limit;
        if (app.current.offset % app.current.limit > 0) {
            app.current.offset = Math.floor(app.current.offset / app.current.limit);
        }
    }
    appGoto(app.current.card, app.current.tab, app.current.view,
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search, 0);
}

/**
 * Endless scrolling function
 * @returns {void}
 */
function setEndlessScroll() {
    const obEl = elGetById(app.id + 'List').lastElementChild;
    if (obEl) {
        const options = {
            root: elGetById(app.id + 'Container'),
            rootMargin: "0px",
            threshold: 0.25,
        };
        const io = new IntersectionObserver(function(entries, observer) {
            if (entries[0].isIntersecting === true) {
                observer.unobserve(entries[0].target);
                const limit =  app.current.limit + settings.webuiSettings.maxElementsPerPage;
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, limit, app.current.filter, app.current.sort,
                    app.current.tag, app.current.search, undefined, true);
            }
        }, options);
        io.observe(obEl);
    }
}

/**
 * Pagination function
 * @param {number} total number of total entries
 * @param {number} returned number of returned entries
 * @returns {void}
 */
function setPagination(total, returned) {
    if (features.featPagination === false) {
        if (returned === app.current.limit ||
            returned === settings.webuiSettings.maxElementsPerPage)
        {
            setEndlessScroll();
        }
        return;
    }
    const curPaginationTop = elGetById(app.id + 'PaginationTop');
    if (curPaginationTop === null) {
        return;
    }

    if (app.current.limit === 0) {
        app.current.limit = maxElementsPerPage;
    }

    let totalPages = total < app.current.limit
        ? total === -1
            ? -1
            : 1
        : Math.ceil(total / app.current.limit);
    const curPage = Math.ceil(app.current.offset / app.current.limit) + 1;
    if (app.current.limit > returned) {
        totalPages = curPage;
    }

    //toolbar
    const paginationTop = createPaginationEls(totalPages, curPage);
    paginationTop.classList.add('me-2');
    paginationTop.setAttribute('id', curPaginationTop.id);
    curPaginationTop.replaceWith(paginationTop);

    //bottom
    const bottomBar = elGetById(app.id + 'ButtonsBottom');
    elClear(bottomBar);
    if (domCache.body.classList.contains('not-mobile') ||
        returned < 25)
    {
        elHide(bottomBar);
        return;
    }
    const toTop = elCreateText('button', {"class": ["btn", "btn-secondary", "mi"],
        "data-title-phrase": "To top"}, 'keyboard_arrow_up');
    toTop.addEventListener('click', function(event) {
        event.preventDefault();
        scrollToPosY(null, 0);
    }, false);
    bottomBar.appendChild(toTop);
    const paginationBottom = createPaginationEls(totalPages, curPage);
    paginationBottom.childNodes[1].classList.add('dropup');
    bottomBar.appendChild(paginationBottom);
    elShow(bottomBar);
}

/**
 * Creates the pagination elements with the dropdown
 * @param {number} totalPages number of total pages
 * @param {number} curPage current page
 * @returns {HTMLElement} button group with pagination
 */
function createPaginationEls(totalPages, curPage) {
    const prev = elCreateText('button', {"data-title-phrase": "Previous page",
        "type": "button", "class": ["btn", "btn-secondary", "mi"]}, 'navigate_before');
    if (curPage === 1) {
        elDisable(prev);
    }
    else {
        prev.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPageDir('prev', undefined);
        }, false);
    }

    const pageDropdownBtn = elCreateText('button', {"type": "button", "data-bs-toggle": "dropdown",
        "class": ["rounded-end-0", "btn", "btn-secondary", "dropdown-toggle", "px-2"]}, curPage.toString());
    const pageDropdownMenu = elCreateEmpty('div', {"class": ["dropdown-menu", "px-2", "dropdownWide"]});

    const row = elCreateNodes('div', {"class": ["row"]}, [
        elCreateTextTn('label', {"class": ["col-8", "col-form-label"]}, 'Elements per page'),
        elCreateEmpty('div', {"class": ["col-4"]})
    ]);

    const elPerPage = elCreateEmpty('select', {"class": ["form-control", "form-select", "border-secondary"]});
    for (const i in settingsWebuiFields.maxElementsPerPage.validValues) {
        elPerPage.appendChild(
            elCreateText('option', {"value": i}, i)
        );
        if (Number(i) === app.current.limit) {
            elPerPage.lastChild.setAttribute('selected', 'selected');
        }
    }
    elPerPage.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    elPerPage.addEventListener('change', function(event) {
        const newLimit = Number(getSelectValue(event.target));
        if (app.current.limit !== newLimit) {
            BSN.Dropdown.getInstance(event.target.parentNode.parentNode.parentNode.previousElementSibling).hide();
            gotoPage(app.current.offset, newLimit);
        }
    }, false);
    row.lastChild.appendChild(elPerPage);

    const pageGrp = elCreateEmpty('div', {"class": ["btn-group"]});

    let start = curPage - 3;
    if (start < 1) {
        start = 1;
    }
    let end = start + 5;
    if (end >= totalPages) {
        end = totalPages - 1;
        start = end - 6 > 1
            ? end - 6
            : 1;
    }

    const first = elCreateEmpty('button', {"data-title-phrase": "First page", "type": "button", "class": ["btn", "btn-secondary"]});
    if (start === 1) {
        first.textContent = '1';
    }
    else {
        first.textContent = 'first_page';
        first.classList.add('mi');
    }
    if (curPage === 1) {
        elDisable(first);
        first.classList.add('active');
    }
    else {
        first.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage(0, undefined);
        }, false);
    }
    pageGrp.appendChild(first);

    if (end > start) {
        for (let i = start; i < end; i++) {
            pageGrp.appendChild(
                elCreateText('button', {"class": ["btn", "btn-secondary"]}, (i + 1).toString())
            );
            if (i + 1 === curPage) {
                pageGrp.lastChild.classList.add('active');
            }
            if (totalPages === -1) {
                elDisable(pageGrp.lastChild);
            }
            else {
                pageGrp.lastChild.addEventListener('click', function(event) {
                    event.preventDefault();
                    gotoPage(i * app.current.limit, undefined);
                }, false);
            }
        }

        const last = elCreateEmpty('button', {"data-title-phrase": "Last page", "type": "button", "class": ["btn", "btn-secondary"]});
        if (totalPages === end + 1) {
            last.textContent = (end + 1).toString();
        }
        else {
            last.textContent = 'last_page';
            last.classList.add('mi');
        }
        if (totalPages === -1) {
            elDisable(last);
        }
        else if (totalPages === curPage) {
            if (curPage !== 1) {
                last.classList.add('active');
            }
            elDisable(last);
        }
        else {
            last.addEventListener('click', function(event) {
                event.preventDefault();
                gotoPage(totalPages * app.current.limit - app.current.limit, undefined);
            }, false);
        }
        pageGrp.appendChild(last);
    }

    pageDropdownMenu.appendChild(
        elCreateNode('div', {"class": ["row", "mb-3"]}, pageGrp)
    );
    pageDropdownMenu.appendChild(row);

    const next = elCreateText('button', {"data-title-phrase": "Next page", "type": "button", "class": ["btn", "btn-secondary", "mi"]}, 'navigate_next');
    if (totalPages !== -1 && totalPages === curPage) {
        elDisable(next);
    }
    else {
        next.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPageDir('next', undefined);
        }, false);
    }

    const outer = elCreateNodes('div', {"class": ["btn-group", "pagination"]}, [
        prev,
        elCreateNodes('div', {"class": ["btn-group"]}, [
            pageDropdownBtn,
            pageDropdownMenu
        ]),
        next
    ]);
    new BSN.Dropdown(pageDropdownBtn);
    return outer;
}
