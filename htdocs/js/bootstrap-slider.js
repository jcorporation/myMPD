(function($){

    function slider(options){
        if(typeof options === 'number'){
            options = $.extend(
                {
                    origVal:options
                },
                defaults,
                {
                    val:(( options < 0 ) ? 0 : ( (options > 100 ) ? 100 : options))
                }
            );
        }
        else if (options === "get"){
            var vals = [];

            $(this).each(function() {
                vals.push($(this).data("sliderValue"));
            });
            return vals;
        }
        else if(typeof options === 'object'){
            options = $.extend({origVal:options.val,origBarColor:options.barColor},defaults,options);
        }

        return $(this).each (function() {
            var self=$(this);

            if(self.hasClass("slider-wrapper-jq")){
                if(self.data("dragSlider") === "true")
                    return;
                if(typeof options.origVal !== "undefined")
                    self.slider._setValue.call(self,options.val,null,true);
                if(typeof options.origBarColor !== "undefined")
                    self.find('.progress-bar').css("background-color",options.barColor);
                return;
            }

            self.addClass("slider-wrapper-jq")
            .append($("<div class='progress' style='position:relative;left:0;height:20px;'/>")
            .append("<div class='progress-bar' style='position:width: 30%;background-color: "+
            options.barColor+"; -webkit-transition:none; transition:none;' />")
            .append("<div class='btn btn-default ' style='position:absolute;height:100%;padding:6px 10px;margin-left:-10px;vertical-align: top'>"));

            self.find('.progress').on('mousedown', function(evt){
                self.data("dragSlider","true")
                .data("startPoint",evt.pageX)
                .data("endPoint",evt.pageX);

                if(!$(evt.target).hasClass("btn")){
                    self.slider._setWidthFromEvent.call(self,evt.pageX,null,true);
                }
                else{
                    self.data("btnTarget","true");
                }

                evt.preventDefault();
                evt.stopPropagation();
            });

            $(window).on('mouseup', function(evt){
                if(self.data("dragSlider")==="true"){
                    if(!(self.data("btnTarget") === "true" && self.data("startPoint") === self.data("endPoint") )){
                        self.slider._setWidthFromEvent.call(self,evt.pageX);
                    }

                    self.removeData("dragSlider")
                    .removeData("btnTarget")
                    .removeData("startPoint")
                    .removeData("endPoint");
                }
            }).on('mousemove',function(evt){
                if(self.data("dragSlider")==="true"){
                    self.slider._setWidthFromEvent.call(self,evt.pageX,null,true);
                    self.data("endPoint",evt.pageX);
                    evt.preventDefault();
                }
            });

            self.slider._setValue.call(self,options.val);
        });

    }

    var defaults={
        val:50,
        barColor:"#28a745"
    },
    _setWidthFromEvent = function(pageX,reqVals,supress) {
        if(!reqVals){
           reqVals =  this.slider._getRequiredValues.call(this);
        }
        else{
            reqVals = null;
        }

        var width = pageX - reqVals.pbar.offset().left,
            perc = ((100.0*width) / (reqVals.progw));

        return this.slider._setValue.call(this,perc,reqVals,supress);
    },
     _setValue = function (val,reqVals,supress) {
        if(!reqVals){
           reqVals =  this.slider._getRequiredValues.call(this);
        }

        val = ((val<0)?0:((val>100)?100:val));
        var adjVal= ((val*(100-reqVals.pbutp)/100) + (reqVals.pbutp/2));

        this.data("sliderValue",val);
        reqVals.pbar.css({width:adjVal+"%"});
        this.find('div.btn').css('left',adjVal+"%");

        if(supress !== true){
            this.trigger("slider.newValue",{val:Math.round(val)});
        }

        return val;
    },
    _getRequiredValues = function(){
        var pbar=this.find('.progress-bar'),
            progw=this.children('.progress').get(0).clientWidth,
            pbutp=((this.find('div.btn').get(0).clientWidth*100.0)/progw);

        return {
            pbar:pbar,
            progw:progw,
            pbutp:pbutp
        };
    };

    $.fn.slider = slider;
    $.fn.slider.defaults = defaults;
    $.fn.slider._getRequiredValues = _getRequiredValues ;
    $.fn.slider._setWidthFromEvent = _setWidthFromEvent;
    $.fn.slider._setValue = _setValue;

})(jQuery);
