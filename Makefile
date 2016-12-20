.PHONY: clean All

All:
	@echo "----------Building project:[ delta_stepping - Release ]----------"
	@cd "delta_stepping" && "$(MAKE)" -f  "delta_stepping.mk"
clean:
	@echo "----------Cleaning project:[ delta_stepping - Release ]----------"
	@cd "delta_stepping" && "$(MAKE)" -f  "delta_stepping.mk" clean
