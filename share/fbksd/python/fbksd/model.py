import uuid

class BaseModel():
    def __init__(self, obj_id):
        self.id = obj_id
        self.uuid = uuid.uuid4()


class Technique(BaseModel):
    def __init__(self, obj_id):
        super().__init__(obj_id)
        self.name = ''
        self.full_name = ''
        self.comment = ''
        self.citation = ''
        self.versions = []


class Filter(Technique):
    __nextId = 1

    def __init__(self):
        super().__init__(Filter.__nextId)
        Filter.__nextId += 1


class Sampler(Technique):
    __nextId = 1

    def __init__(self):
        super().__init__(Sampler.__nextId)
        Sampler.__nextId += 1


class TechniqueVersion(BaseModel):
    def __init__(self, obj_id):
        super().__init__(obj_id)
        self.technique = None
        self.tag = ''
        self.executable = ''
        self.message = ''
        self.status = ''
        self.results = [] # Result[]

    def has_result(self, scene, spp):
        return any([True for result in self.results if result.scene is scene and result.spp == spp])

    def get_result(self, scene, spp):
        results = [result for result in self.results if result.scene is scene and result.spp == spp]
        if results:
            return results[0]
        else:
            return None

    def get_results(self, scene):
        r_results = []
        for r in self.results:
            if r.scene is scene:
                r_results.append(r)
        return r_results

    def get_name(self):
        return '{}-{}'.format(self.technique.name, self.tag)


class FilterVersion(TechniqueVersion):
    __nextId = 1

    def __init__(self):
        super().__init__(FilterVersion.__nextId)
        FilterVersion.__nextId += 1


class SamplerVersion(TechniqueVersion):
    __nextId = 1

    def __init__(self):
        super().__init__(SamplerVersion.__nextId)
        SamplerVersion.__nextId += 1


class Renderer(BaseModel):
    __nextId = 1

    def __init__(self):
        super().__init__(Renderer.__nextId)
        Renderer.__nextId += 1

        self.name = ''
        self.path = ''
        self.scenes = [] # Scene[]


class Scene(BaseModel):
    __nextId = 1

    def __init__(self):
        super().__init__(Scene.__nextId)
        Scene.__nextId += 1

        self.name = ''
        self.path = ''
        self.ground_truth = ''
        self.dof_w = 0.0 # depth-of-field weight
        self.mb_w = 0.0 # motion blur
        self.ss_w = 0.0 # soft shadow
        self.glossy_w = 0.0 # glossy
        self.gi_w = 0.0 # global illumination
        self.renderer = None # Renderer
        self.spps = []
        self.regions = [] # ImageRegion


class ImageRegion(BaseModel):
    __nextId = 1

    def __init__(self):
        super().__init__(ImageRegion.__nextId)
        ImageRegion.__nextId += 1

        self.xmin = 0
        self.ymin = 0
        self.xmax = 0
        self.ymax = 0
        # Weights for each noise source (0.0 - 1.0)
        self.dof_w = 0.0 # depth-of-field weight
        self.mb_w = 0.0 # motion blur
        self.ss_w = 0.0 # soft shadow
        self.glossy_w = 0.0 # glossy
        self.gi_w = 0.0 # global illumination


class RegionError(BaseModel):
    __nextId = 1

    def __init__(self):
        super().__init__(RegionError.__nextId)
        RegionError.__nextId += 1

        self.mse = 0.0
        self.psnr = 0.0
        self.ssim = 0.0
        self.rmse = 0.0
        self.region = None # ImageRegion
        self.result = None # Result


class TechniqueResult(BaseModel):
    def __init__(self, obj_id):
        super().__init__(obj_id)

        self.scene = None
        self.spp = 0
        self.exec_time = 0
        self.rendering_time = 0
        self.mse = 0.0
        self.psnr = 0.0
        self.ssim = 0.0
        self.rmse = 0.0
        self.aborted = False
        self.region_errors = [] # RegionError[]


class Result(TechniqueResult):
    __nextId = 1

    def __init__(self):
        super().__init__(Result.__nextId)
        Result.__nextId += 1
        self.filter_version = None # SamplerVersion


class SamplerResult(TechniqueResult):
    __nextId = 1

    def __init__(self):
        super().__init__(SamplerResult.__nextId)
        SamplerResult.__nextId += 1
        self.sampler_version = None # FilterVersion


class PersistentState:
    def __init__(self):
        pass
