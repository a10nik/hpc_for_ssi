.PHONY: clean All

All:
	@echo "----------Building project:[ multi_shooting - Release ]----------"
	@cd "multi_shooting" && "$(MAKE)" -f  "multi_shooting.mk"
clean:
	@echo "----------Cleaning project:[ multi_shooting - Release ]----------"
	@cd "multi_shooting" && "$(MAKE)" -f  "multi_shooting.mk" clean
